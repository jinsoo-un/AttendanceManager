#include "attendance.h"
#include <fstream>
#include <cctype>

static std::string toLowerCopy(const std::string& s) {
    std::string o; o.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) { unsigned char c = (unsigned char)s[i]; o.push_back((char)std::tolower(c)); }
    return o;
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

// ThresholdGradePolicy
ThresholdGradePolicy::ThresholdGradePolicy() {
    GradeBand g;
    g.gradeName = "GOLD"; g.minScore = 50; bands_.push_back(g);
    g.gradeName = "SILVER"; g.minScore = 30; bands_.push_back(g);
    g.gradeName = "NORMAL"; g.minScore = 0;  bands_.push_back(g);
}

ThresholdGradePolicy::ThresholdGradePolicy(const std::vector<GradeBand>& b) :bands_(b) {}
std::string ThresholdGradePolicy::decide(int totalPoints) const {
    for (size_t i = 0; i < bands_.size(); ++i) { if (totalPoints >= bands_[i].minScore) return bands_[i].gradeName; }
    return "UNDEFINED";
}

// DefaultScoringPolicy
DefaultScoringPolicy::DefaultScoringPolicy() {
    base_[0] = 1; base_[1] = 1; base_[2] = 3; base_[3] = 1; base_[4] = 1; base_[5] = 2; base_[6] = 2;
    wedBonusThreshold_ = 10; wedBonus_ = 10;
    weekendBonusThreshold_ = 10; weekendBonus_ = 10;
}

int DefaultScoringPolicy::basePoint(Weekday d) const { return base_[(int)d]; }

int DefaultScoringPolicy::bonusPoints(const PlayerStat& p) const {
    int bonus = 0;
    if (p.dayCount[(int)Wed] >= wedBonusThreshold_) bonus += wedBonus_;
    int wk = p.dayCount[(int)Sat] + p.dayCount[(int)Sun];
    if (wk >= weekendBonusThreshold_) bonus += weekendBonus_;
    return bonus;
}

// Elimination
bool NormalNoWedWeekendElimination::isEliminated(const PlayerStat& p) const {
    bool neverWedOrWeekend = (p.dayCount[(int)Wed] == 0) && (p.dayCount[(int)Sat] == 0) && (p.dayCount[(int)Sun] == 0);
    return (p.grade == "NORMAL") && neverWedOrWeekend;
}

// AttendanceSystem (Facade)
AttendanceSystem::AttendanceSystem()
    : scoring_(0), grade_(0), elimination_(0),
    ownScoring_(false), ownGrade_(false), ownElim_(false)
{
    scoring_ = new DefaultScoringPolicy(); ownScoring_ = true;
    grade_ = new ThresholdGradePolicy(); ownGrade_ = true;
    elimination_ = new NormalNoWedWeekendElimination(); ownElim_ = true;
}

AttendanceSystem::AttendanceSystem(IScoringPolicy* s, IGradePolicy* g, IEliminationRule* e)
    : scoring_(s), grade_(g), elimination_(e),
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
    PlayerStat p; p.id = idx + 1; p.name = name; players_.push_back(p);
    return idx;
}

void AttendanceSystem::addRecord(const std::string& name, Weekday day) {
    int idx = ensurePlayerIndex(name);
    PlayerStat& p = players_[idx];
    p.dayCount[(int)day] += 1;
    p.basePoints += scoring_->basePoint(day);
}

bool AttendanceSystem::addRecordLine(const std::string& nameToken, const std::string& dayToken) {
    Weekday w; if (!parseWeekday(dayToken, w)) return false; addRecord(nameToken, w); return true;
}

void AttendanceSystem::loadFromStream(std::istream& in) {
    std::string name, day; while (in >> name >> day) { addRecordLine(name, day); }
}

void AttendanceSystem::loadFromFile(const std::string& path) {
    std::ifstream fin(path.c_str()); if (!fin.is_open()) { std::cerr << "Failed to open file: " << path << "\n"; return; }
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
        os << "NAME : " << p.name << ", POINT : " << p.totalPoints << ", GRADE : " << p.grade << "\n";
    }
    os << "\nRemoved player\n==============\n";
    for (size_t i = 0; i < players_.size(); ++i) {
        const PlayerStat& p = players_[i];
        if (p.eliminationCandidate) os << p.name << "\n";
    }
}

void AttendanceSystem::clear() { indexByName_.clear(); players_.clear(); }
