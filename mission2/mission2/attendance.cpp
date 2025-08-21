#include "attendance.h"
#include <fstream>
#include <cctype>

// --- 로컬 유틸 ---
static std::string toLowerCopy(const std::string& s) {
    std::string out; out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

bool parseWeekday(const std::string& s, Weekday& out) {
    std::string t = toLowerCopy(s);
    if (t == "monday") { out = Mon; return true; }
    if (t == "tuesday") { out = Tue; return true; }
    if (t == "wednesday") { out = Wed; return true; }
    if (t == "thursday") { out = Thu; return true; }
    if (t == "friday") { out = Fri; return true; }
    if (t == "saturday") { out = Sat; return true; }
    if (t == "sunday") { out = Sun; return true; }
    return false;
}

int computeBasePointForDay(Weekday d, const ScoringPolicy& policy) {
    return policy.basePointsByDay[(int)d];
}

int computeBonusPoints(const PlayerStat& p, const ScoringPolicy& policy) {
    int bonus = 0;
    if (p.dayCount[(int)Wed] >= policy.wednesdayBonusThreshold)
        bonus += policy.wednesdayBonusPoints;

    int weekendTotal = p.dayCount[(int)Sat] + p.dayCount[(int)Sun];
    if (weekendTotal >= policy.weekendBonusThreshold)
        bonus += policy.weekendBonusPoints;
    return bonus;
}

std::string decideGrade(int totalPoints, const GradePolicy& gradePolicy) {
    for (size_t i = 0; i < gradePolicy.bands.size(); ++i) {
        if (totalPoints >= gradePolicy.bands[i].minScore)
            return gradePolicy.bands[i].gradeName;
    }
    return "UNDEFINED";
}

bool isEliminationCandidate(const PlayerStat& p) {
    bool neverWedOrWeekend = (p.dayCount[(int)Wed] == 0) &&
        (p.dayCount[(int)Sat] == 0) &&
        (p.dayCount[(int)Sun] == 0);
    return (p.grade == "NORMAL") && neverWedOrWeekend;
}

// --- AttendanceSystem 구현 ---
AttendanceSystem::AttendanceSystem() : scoringPolicy_(), gradePolicy_() {}

AttendanceSystem::AttendanceSystem(const ScoringPolicy& sp, const GradePolicy& gp)
    : scoringPolicy_(sp), gradePolicy_(gp) {
}

int AttendanceSystem::ensurePlayerIndex(const std::string& name) {
    std::map<std::string, int>::iterator it = indexByName_.find(name);
    if (it != indexByName_.end()) return it->second;

    int idx = (int)players_.size();
    indexByName_.insert(std::make_pair(name, idx));
    PlayerStat p;
    p.id = idx + 1;
    p.name = name;
    players_.push_back(p);
    return idx;
}

void AttendanceSystem::addRecord(const std::string& name, Weekday day) {
    int idx = ensurePlayerIndex(name);
    PlayerStat& p = players_[idx];
    int d = (int)day;
    p.dayCount[d] += 1;
    p.basePoints += computeBasePointForDay(day, scoringPolicy_);
}

bool AttendanceSystem::addRecordLine(const std::string& nameToken, const std::string& dayToken) {
    Weekday w;
    if (!parseWeekday(dayToken, w)) return false;
    addRecord(nameToken, w);
    return true;
}

void AttendanceSystem::loadFromStream(std::istream& in) {
    std::string name, dayStr;
    while (in >> name >> dayStr) {
        addRecordLine(name, dayStr); // invalid 요일은 무시
    }
}

void AttendanceSystem::loadFromFile(const std::string& path) {
    std::ifstream fin(path.c_str());
    if (!fin.is_open()) {
        std::cerr << "Failed to open file: " << path << "\n";
        return;
    }
    loadFromStream(fin);
}

void AttendanceSystem::compute() {
    for (size_t i = 0; i < players_.size(); ++i) {
        PlayerStat& p = players_[i];
        p.wedCount = p.dayCount[(int)Wed];
        p.weekendCount = p.dayCount[(int)Sat] + p.dayCount[(int)Sun];
        p.bonusPoints = computeBonusPoints(p, scoringPolicy_);
        p.totalPoints = p.basePoints + p.bonusPoints;
        p.grade = decideGrade(p.totalPoints, gradePolicy_);
        p.eliminationCandidate = isEliminationCandidate(p);
    }
}

const std::vector<PlayerStat>& AttendanceSystem::players() const {
    return players_;
}

void AttendanceSystem::printSummary(std::ostream& os) const {
    for (size_t i = 0; i < players_.size(); ++i) {
        const PlayerStat& p = players_[i];
        os << "NAME : " << p.name
            << ", POINT : " << p.totalPoints
            << ", GRADE : " << p.grade << "\n";
    }
    os << "\nRemoved player\n";
    os << "==============\n";
    for (size_t i = 0; i < players_.size(); ++i) {
        const PlayerStat& p = players_[i];
        if (p.eliminationCandidate) os << p.name << "\n";
    }
}

void AttendanceSystem::clear() {
    indexByName_.clear();
    players_.clear();
}
