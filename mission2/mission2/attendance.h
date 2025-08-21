#pragma once
#include <map>
#include <string>
#include <vector>
#include <iostream>

struct ScoringPolicy {
    int basePointsByDay[7];
    int wednesdayBonusThreshold;
    int wednesdayBonusPoints;
    int weekendBonusThreshold;
    int weekendBonusPoints;

    ScoringPolicy() {
        basePointsByDay[0] = 1; // Mon
        basePointsByDay[1] = 1; // Tue
        basePointsByDay[2] = 3; // Wed
        basePointsByDay[3] = 1; // Thu
        basePointsByDay[4] = 1; // Fri
        basePointsByDay[5] = 2; // Sat
        basePointsByDay[6] = 2; // Sun

        wednesdayBonusThreshold = 10;
        wednesdayBonusPoints = 10;
        weekendBonusThreshold = 10;
        weekendBonusPoints = 10;
    }
};

struct GradeBand {
    std::string gradeName; // "GOLD", "SILVER", "NORMAL"
    int minScore;
};

struct GradePolicy {
    std::vector<GradeBand> bands;
    GradePolicy() {
        GradeBand g;
        g.gradeName = "GOLD";   g.minScore = 50; bands.push_back(g);
        g.gradeName = "SILVER"; g.minScore = 30; bands.push_back(g);
        g.gradeName = "NORMAL"; g.minScore = 0;  bands.push_back(g);
    }
};

enum Weekday { Mon = 0, Tue, Wed, Thu, Fri, Sat, Sun };

struct PlayerStat {
    int id;
    std::string name;
    int dayCount[7];
    int wedCount;
    int weekendCount;
    int basePoints;
    int bonusPoints;
    int totalPoints;
    std::string grade;
    bool eliminationCandidate;

    PlayerStat() : id(0), name(""), wedCount(0), weekendCount(0),
        basePoints(0), bonusPoints(0), totalPoints(0),
        grade(""), eliminationCandidate(false) {
        for (int i = 0; i < 7; ++i) dayCount[i] = 0;
    }
};

struct Record {
    std::string name;
    Weekday day;
    Record() {}
    Record(const std::string& n, Weekday d) : name(n), day(d) {}
};

// --- 순수 함수 유틸 ---
bool parseWeekday(const std::string& s, Weekday& out);
int  computeBasePointForDay(Weekday d, const ScoringPolicy& policy);
int  computeBonusPoints(const PlayerStat& p, const ScoringPolicy& policy);
std::string decideGrade(int totalPoints, const GradePolicy& gradePolicy);
bool isEliminationCandidate(const PlayerStat& p);

class AttendanceSystem {
public:
    AttendanceSystem();
    explicit AttendanceSystem(const ScoringPolicy& sp, const GradePolicy& gp);

    // 입력
    void addRecord(const std::string& name, Weekday day);
    bool addRecordLine(const std::string& nameToken, const std::string& dayToken); // 파싱 포함

    // 배치 입력
    void loadFromStream(std::istream& in);
    void loadFromFile(const std::string& path);

    // 계산
    void compute(); // base/bonus/grade/elimination 산출

    // 조회/출력
    const std::vector<PlayerStat>& players() const;
    void printSummary(std::ostream& os) const;

    // 테스트 편의용
    void clear();

private:
    ScoringPolicy scoringPolicy_;
    GradePolicy   gradePolicy_;
    std::map<std::string, int> indexByName_;
    std::vector<PlayerStat> players_;

    int ensurePlayerIndex(const std::string& name);
};
