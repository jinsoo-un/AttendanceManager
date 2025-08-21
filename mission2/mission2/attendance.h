#pragma once

#include <map>
#include <string>
#include <vector>
#include <iostream>

enum Weekday { Mon = 0, Tue, Wed, Thu, Fri, Sat, Sun };

bool parseWeekday(const std::string& s, Weekday& out);

struct PlayerStat;

// Strategy Interfaces
struct IScoringPolicy {
    virtual ~IScoringPolicy() {}
    virtual int basePoint(Weekday d) const = 0;
    virtual int bonusPoints(const PlayerStat& p) const = 0;
};

struct IGradePolicy {
    virtual ~IGradePolicy() {}
    virtual std::string decide(int totalPoints) const = 0;
};

struct IEliminationRule {
    virtual ~IEliminationRule() {}
    virtual bool isEliminated(const PlayerStat& p) const = 0;
};

struct GradeBand {
    std::string gradeName;
    int minScore;
};

class ThresholdGradePolicy : public IGradePolicy {
public:
    ThresholdGradePolicy(); // GOLD 50, SILVER 30, NORMAL 0
    explicit ThresholdGradePolicy(const std::vector<GradeBand>& bands);
    virtual std::string decide(int totalPoints) const;
private:
    std::vector<GradeBand> bands_;
};

class DefaultScoringPolicy : public IScoringPolicy {
public:
    DefaultScoringPolicy(); // Mon/Tue/Thu/Fri=1, Wed=3, Sat/Sun=2 + Wed>=10 +10, Weekend>=10 +10
    virtual int basePoint(Weekday d) const;
    virtual int bonusPoints(const PlayerStat& p) const;
private:
    int base_[7];
    int wedBonusThreshold_, wedBonus_;
    int weekendBonusThreshold_, weekendBonus_;
};

class NormalNoWedWeekendElimination : public IEliminationRule {
public:
    virtual bool isEliminated(const PlayerStat& p) const;
};

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

// Facade
class AttendanceSystem {
public:
    // 기본 생성자: 기본 정책을 내부에서 생성해 장착 (클라이언트 변경 불필요)
    AttendanceSystem();

    // 전략 주입 생성자 (소유권은 호출자가 가짐)
    AttendanceSystem(IScoringPolicy* scoring,
        IGradePolicy* grade,
        IEliminationRule* elimination);

    ~AttendanceSystem();

    // Input
    void addRecord(const std::string& name, Weekday day);
    bool addRecordLine(const std::string& nameToken, const std::string& dayToken);
    void loadFromStream(std::istream& in);
    void loadFromFile(const std::string& path);

    // Compute
    void compute();

    // Output
    const std::vector<PlayerStat>& players() const;
    void printSummary(std::ostream& os) const;

    // Utils
    void clear();

private:
    IScoringPolicy* scoring_;
    IGradePolicy* grade_;
    IEliminationRule* elimination_;

    bool ownScoring_, ownGrade_, ownElim_;

    std::map<std::string, int> indexByName_;
    std::vector<PlayerStat>   players_;

    int ensurePlayerIndex(const std::string& name);

    AttendanceSystem(const AttendanceSystem&);
    AttendanceSystem& operator=(const AttendanceSystem&);
};
