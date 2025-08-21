#include "attendance.h"
#include <fstream>
#include <cctype>

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

// 기본 정책
ThresholdGradePolicy::ThresholdGradePolicy() {
    GradeBand g;
    g.gradeName = "GOLD";   g.minScore = 50; bands_.push_back(g);
    g.gradeName = "SILVER"; g.minScore = 30; bands_.push_back(g);
    g.gradeName = "NORMAL"; g.minScore = 0;  bands_.push_back(g);
}
ThresholdGradePolicy::ThresholdGradePolicy(const std::vector<GradeBand>& bands)
    : bands_(bands) {
}
std::string ThresholdGradePolicy::decide(int totalPoints) const {
    for (size_t i = 0; i < bands_.size(); ++i) {
        if (totalPoints >= bands_[i].minScore) return bands_[i].gradeName;
    }
    return "UNDEFINED";
}

WednesdayBonusRule::WednesdayBonusRule(int threshold, int points)
    : threshold_(threshold), points_(points) {
}
int WednesdayBonusRule::compute(const PlayerStat& p) const {
    return (p.dayCount[(int)Wed] >= threshold_) ? points_ : 0;
}

WeekendBonusRule::WeekendBonusRule(int threshold, int points)
    : threshold_(threshold), points_(points) {
}
int WeekendBonusRule::compute(const PlayerStat& p) const {
    int weekendTotal = p.dayCount[(int)Sat] + p.dayCount[(int)Sun];
    return (weekendTotal >= threshold_) ? points_ : 0;
}

CompositeScoringPolicy::CompositeScoringPolicy() {
    // 기본 요일 점수
    base_[0] = 1; base_[1] = 1; base_[2] = 3; base_[3] = 1; base_[4] = 1; base_[5] = 2; base_[6] = 2;
    // 기본 보너스 규칙(수/주말 10회 이상 +10)
    static WednesdayBonusRule wed10(10, 10);
    static WeekendBonusRule   wk10(10, 10);
    rules_.push_back(&wed10);
    rules_.push_back(&wk10);
}
CompositeScoringPolicy::CompositeScoringPolicy(const int basePointsByDay[7],
    const std::vector<IBonusRule*>& rules) {
    for (int i = 0; i < 7; ++i) base_[i] = basePointsByDay[i];
    rules_ = rules;
}
int CompositeScoringPolicy::basePoint(Weekday d) const { return base_[(int)d]; }
int CompositeScoringPolicy::bonusPoints(const PlayerStat& p) const {
    int sum = 0;
    for (size_t i = 0; i < rules_.size(); ++i) sum += rules_[i]->compute(p);
    return sum;
}

bool NormalNoWedWeekendElimination::isEliminated(const PlayerStat& p) const {
    bool neverWedOrWeekend = (p.dayCount[(int)Wed] == 0) &&
        (p.dayCount[(int)Sat] == 0) &&
        (p.dayCount[(int)Sun] == 0);
    return (p.grade == "NORMAL") && neverWedOrWeekend;
}

// ============ AttendanceSystem ============
AttendanceSystem::AttendanceSystem()
    : scoring_(0), grade_(0), elimination_(0),
    ownScoring_(false), ownGrade_(false), ownElim_(false)
{
    // 기본 정책들을 내부 생성하여 장착 (클라이언트 변경 불필요)
    scoring_ = new CompositeScoringPolicy(); ownScoring_ = true;
    grade_ = new ThresholdGradePolicy();   ownGrade_ = true;
    elimination_ = new NormalNoWedWeekendElimination(); ownElim_ = true;
}

AttendanceSystem::AttendanceSystem(IScoringPolicy* scoring,
    IGradePolicy* grade,
    IEliminationRule* elimination)
    : scoring_(scoring), grade_(grade), elimination_(elimination),
    ownScoring_(false), ownGrade_(false), ownElim_(false) {
}

AttendanceSystem::~AttendanceSystem() {
    if (ownScoring_) delete scoring_;
    if (ownGrade_)   delete grade_;
    if (ownElim_)    delete elimination_;
}

int AttendanceSystem::ensurePlayerIndex(const std::string& name) {
    std::map<std::string, int>::iterator it = indexByName_.find(name);
    if (it != indexByName_.end()) return it->second;
    int idx = (int)players_.size();
    indexByName_.insert(std::make_pair(name, idx));
    PlayerStat p; p.id = idx + 1; p.name = name;
    players_.push_back(p);
    return idx;
}

void AttendanceSystem::addRecord(const std::string& name, Weekday day) {
    int idx = ensurePlayerIndex(name);
    PlayerStat& p = players_[idx];
    p.dayCount[(int)day] += 1;
    p.basePoints += scoring_->basePoint(day);
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
        addRecordLine(name, dayStr);
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
        p.bonusPoints = scoring_->bonusPoints(p);
        p.totalPoints = p.basePoints + p.bonusPoints;
        p.grade = grade_->decide(p.totalPoints);
        p.eliminationCandidate = elimination_->isEliminated(p);
    }
}

const std::vector<PlayerStat>& AttendanceSystem::players() const { return players_; }

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
