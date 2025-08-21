#pragma once
#include <map>
#include <string>
#include <vector>
#include <iostream>

enum Weekday { Mon = 0, Tue, Wed, Thu, Fri, Sat, Sun };

bool parseWeekday(const std::string& s, Weekday& out);

// 확장성을 고려한 정책 인터페이스 추가
struct PlayerStat;

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
    ThresholdGradePolicy(); // 기본(GOLD 50, SILVER 30, NORMAL 0)
    explicit ThresholdGradePolicy(const std::vector<GradeBand>& bands);
    virtual std::string decide(int totalPoints) const;
private:
    std::vector<GradeBand> bands_;
};

// 보너스 규칙을 조합할 수 있게 분리
class IBonusRule {
public:
    virtual ~IBonusRule() {}
    virtual int compute(const PlayerStat& p) const = 0;
};

class WednesdayBonusRule : public IBonusRule {
public:
    WednesdayBonusRule(int threshold, int points);
    virtual int compute(const PlayerStat& p) const;
private:
    int threshold_;
    int points_;
};

class WeekendBonusRule : public IBonusRule {
public:
    WeekendBonusRule(int threshold, int points);
    virtual int compute(const PlayerStat& p) const;
private:
    int threshold_;
    int points_;
};

// 합성 점수 정책: 요일별 기본점수 + 보너스 규칙 집합
class CompositeScoringPolicy : public IScoringPolicy {
public:
    CompositeScoringPolicy(); // 기본: Mon/Tue/Thu/Fri=1, Wed=3, Sat/Sun=2 + 수/주말 보너스 10점(≥10회)
    CompositeScoringPolicy(const int basePointsByDay[7],
        const std::vector<IBonusRule*>& rules);
    virtual int basePoint(Weekday d) const;
    virtual int bonusPoints(const PlayerStat& p) const;
private:
    int base_[7];
    std::vector<IBonusRule*> rules_;
};

// 기본 탈락 규칙: NORMAL 이고 수/주말 모두 0회
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

struct Record {
    std::string name;
    Weekday day;
    Record() {}
    Record(const std::string& n, Weekday d) : name(n), day(d) {}
};

class AttendanceSystem {
public:
    // 기본 생성자: 내부에 기본 정책들을 생성하여 장착(클라이언트 변경 없이 동작)
    AttendanceSystem();

    // 정책 주입 생성자: 외부에서 전략을 넣어 교체 가능(소유권은 호출자가 가짐)
    AttendanceSystem(IScoringPolicy* scoring,
                     IGradePolicy* grade,
                     IEliminationRule* elimination);

    ~AttendanceSystem(); // 기본 생성자로 만든 내부 정책은 파괴 시 정리

    // 입력
    void addRecord(const std::string& name, Weekday day);
    bool addRecordLine(const std::string& nameToken, const std::string& dayToken);

    // 배치 입력
    void loadFromStream(std::istream& in);
    void loadFromFile(const std::string& path);

    // 계산
    void compute();

    // 조회/출력
    const std::vector<PlayerStat>& players() const;
    void printSummary(std::ostream& os) const;

    // 테스트/재사용 편의
    void clear();

private:
    IScoringPolicy* scoring_;
    IGradePolicy* grade_;
    IEliminationRule* elimination_;

    bool ownScoring_;
    bool ownGrade_;
    bool ownElim_;

    std::map<std::string, int> indexByName_;
    std::vector<PlayerStat>   players_;

    int ensurePlayerIndex(const std::string& name);

    AttendanceSystem(const AttendanceSystem&);
    AttendanceSystem& operator=(const AttendanceSystem&);
};
