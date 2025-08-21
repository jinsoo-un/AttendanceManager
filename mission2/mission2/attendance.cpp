#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace std;

enum Weekday { Mon = 0, Tue, Wed, Thu, Fri, Sat, Sun };

static bool parseWeekday(const string& s, Weekday& out) {
    if (s == "monday") { out = Mon; return true; }
    if (s == "tuesday") { out = Tue; return true; }
    if (s == "wednesday") { out = Wed; return true; }
    if (s == "thursday") { out = Thu; return true; }
    if (s == "friday") { out = Fri; return true; }
    if (s == "saturday") { out = Sat; return true; }
    if (s == "sunday") { out = Sun; return true; }
    return false;
}

static bool isWeekend(Weekday d) { return d == Sat || d == Sun; }

// -------- Policy ----------
struct ScoringPolicy {
    int basePointsByDay[7];
    int wednesdayBonusThreshold;
    int wednesdayBonusPoints;
    int weekendBonusThreshold;
    int weekendBonusPoints;

    ScoringPolicy() {
        // Mon, Tue, Wed, Thu, Fri, Sat, Sun
        basePointsByDay[0] = 1;
        basePointsByDay[1] = 1;
        basePointsByDay[2] = 3;
        basePointsByDay[3] = 1;
        basePointsByDay[4] = 1;
        basePointsByDay[5] = 2;
        basePointsByDay[6] = 2;

        wednesdayBonusThreshold = 10;
        wednesdayBonusPoints = 10;
        weekendBonusThreshold = 10;
        weekendBonusPoints = 10;
    }
};

// 등급의 종류는 팀 정책에 따라 추가될 수 있음.
struct GradeBand {
    string gradeName; // "GOLD", "SILVER", "NORMAL"
    int minScore;
};

struct GradePolicy {
    vector<GradeBand> bands;

    GradePolicy() {
        GradeBand g;
        g.gradeName = "GOLD"; g.minScore = 50; bands.push_back(g);
        g.gradeName = "SILVER"; g.minScore = 30; bands.push_back(g);
        g.gradeName = "NORMAL"; g.minScore = 0;  bands.push_back(g);
    }
};

struct PlayerStat {
    int id;
    string name;
    int dayCount[7];
    int wedCount;
    int weekendCount;
    int basePoints;
    int bonusPoints;
    int totalPoints;
    string grade;
    bool eliminationCandidate;

    PlayerStat() : id(0), name(""), wedCount(0), weekendCount(0),
        basePoints(0), bonusPoints(0), totalPoints(0),
        grade(""), eliminationCandidate(false) {
        for (int i = 0; i < 7; ++i) dayCount[i] = 0;
    }
};

struct Record {
    string name;
    Weekday day;
    Record() {}
    Record(const string& n, Weekday d) : name(n), day(d) {}
};

static int computeBasePointForDay(Weekday d, const ScoringPolicy& policy) {
    return policy.basePointsByDay[(int)d];
}

static int computeBonusPoints(const PlayerStat& p, const ScoringPolicy& policy) {
    int bonus = 0;
    if (p.dayCount[(int)Wed] >= policy.wednesdayBonusThreshold)
        bonus += policy.wednesdayBonusPoints;

    int weekendTotal = p.dayCount[(int)Sat] + p.dayCount[(int)Sun];
    if (weekendTotal >= policy.weekendBonusThreshold)
        bonus += policy.weekendBonusPoints;

    return bonus;
}

static string decideGrade(int totalPoints, const GradePolicy& gradePolicy) {
    for (size_t i = 0; i < gradePolicy.bands.size(); ++i) {
        if (totalPoints >= gradePolicy.bands[i].minScore)
            return gradePolicy.bands[i].gradeName;
    }
    return "UNDEFINED";
}

static bool isEliminationCandidate(const PlayerStat& p) {
    bool neverWedOrWeekend = (p.dayCount[(int)Wed] == 0) &&
        (p.dayCount[(int)Sat] == 0) &&
        (p.dayCount[(int)Sun] == 0);
    return (p.grade == "NORMAL") && neverWedOrWeekend;
}

static int ensurePlayerIndex(const string& name,
    map<string, int>& indexByName,
    vector<PlayerStat>& players)
{
    map<string, int>::iterator it = indexByName.find(name);
    if (it != indexByName.end()) return it->second;

    int idx = (int)players.size();
    indexByName.insert(make_pair(name, idx));
    PlayerStat p;
    p.id = idx + 1; // 1부터 시작
    p.name = name;
    players.push_back(p);
    return idx;
}

struct ProcessResult {
    vector<PlayerStat> players;
};

static ProcessResult processAttendance(
    const vector<Record>& records,
    const ScoringPolicy& scoringPolicy,
    const GradePolicy& gradePolicy
) {
    map<string, int> indexByName;
    vector<PlayerStat> players;
    players.reserve(64);

    // 1) 집계
    for (size_t i = 0; i < records.size(); ++i) {
        const string& name = records[i].name;
        Weekday day = records[i].day;
        int idx = ensurePlayerIndex(name, indexByName, players);

        PlayerStat& p = players[idx];
        int d = (int)day;
        p.dayCount[d] += 1;
        p.basePoints += computeBasePointForDay(day, scoringPolicy);
    }

    // 2) 통계/보너스/총점/등급/탈락
    for (size_t i = 0; i < players.size(); ++i) {
        PlayerStat& p = players[i];
        p.wedCount = p.dayCount[(int)Wed];
        p.weekendCount = p.dayCount[(int)Sat] + p.dayCount[(int)Sun];

        p.bonusPoints = computeBonusPoints(p, scoringPolicy);
        p.totalPoints = p.basePoints + p.bonusPoints;
        p.grade = decideGrade(p.totalPoints, gradePolicy);
        p.eliminationCandidate = isEliminationCandidate(p);
    }

    ProcessResult r;
    r.players = players;
    return r;
}

static vector<Record> readRecordsFromFile(const string& path) {
    vector<Record> out;
    ifstream fin(path.c_str());
    if (!fin.is_open()) {
        cerr << "Failed to open file: " << path << "\n";
        return out;
    }

    string name, dayStr;
    while (fin >> name >> dayStr) {
        Weekday w;
        if (!parseWeekday(dayStr, w)) {
            cerr << "Skip invalid weekday token: " << dayStr << " for name " << name << "\n";
            continue;
        }
        out.push_back(Record(name, w));
    }
    return out;
}

static void printSummary(const vector<PlayerStat>& players) {
    for (size_t i = 0; i < players.size(); ++i) {
        const PlayerStat& p = players[i];
        cout << "NAME : " << p.name
            << ", POINT : " << p.totalPoints
            << ", GRADE : " << p.grade << "\n";
    }

    cout << "\nRemoved player\n";
    cout << "==============\n";
    for (size_t i = 0; i < players.size(); ++i) {
        const PlayerStat& p = players[i];
        if (p.eliminationCandidate) cout << p.name << "\n";
    }
}

int main() {
    const string inputPath = "attendance_weekday_500.txt";
    ScoringPolicy scoringPolicy;
    GradePolicy gradePolicy;

    vector<Record> records = readRecordsFromFile(inputPath);
    ProcessResult result = processAttendance(records, scoringPolicy, gradePolicy);
    printSummary(result.players);
    return 0;
}
