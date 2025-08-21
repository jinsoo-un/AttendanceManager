#include "attendance.h"
#include <gtest/gtest.h>
#include <sstream>

// 요일 파싱 테스트
TEST(ParseWeekdayTest, ValidDays) {
    Weekday w;
    EXPECT_TRUE(parseWeekday("monday", w));     EXPECT_EQ(Mon, w);
    EXPECT_TRUE(parseWeekday("wednesday", w));  EXPECT_EQ(Wed, w);
    EXPECT_TRUE(parseWeekday("sunday", w));     EXPECT_EQ(Sun, w);
}

TEST(ParseWeekdayTest, InvalidDay) {
    Weekday w;
    EXPECT_FALSE(parseWeekday("funday", w));
}

// ID 할당 순서 테스트
TEST(AttendanceSystemTest, IdAssignmentOrder) {
    AttendanceSystem sys;           // 기본 정책 자동 장착
    sys.addRecord("Alice", Mon);    // id=1
    sys.addRecord("Bob", Tue);      // id=2
    sys.addRecord("Alice", Wed);    // id=1
    sys.compute();

    const std::vector<PlayerStat>& ps = sys.players();
    ASSERT_EQ(2u, ps.size());
    EXPECT_EQ(1, ps[0].id);
    EXPECT_EQ("Alice", ps[0].name);
    EXPECT_EQ(2, ps[0].dayCount[Mon] + ps[0].dayCount[Wed]);
    EXPECT_EQ(2, ps[1].id);
    EXPECT_EQ("Bob", ps[1].name);
}

// 스코어 계산 테스트
TEST(AttendanceSystemTest, BaseAndBonus) {
    // 기존: ScoringPolicy sp; GradePolicy gp; AttendanceSystem sys(sp, gp);
    // 변경: 전략 주입
    CompositeScoringPolicy scoring;
    ThresholdGradePolicy   grade;
    NormalNoWedWeekendElimination elim;
    AttendanceSystem sys(&scoring, &grade, &elim);

    // Alice: Wed 10회 -> 수요일 보너스 +10, 각 수요일 기본점수 3*10 = 30
    for (int i = 0; i < 10; ++i) sys.addRecord("Alice", Wed);

    // Bob: Sat 10회 -> 주말 보너스 +10, 각 토요일 기본점수 2*10 = 20
    for (int i = 0; i < 10; ++i) sys.addRecord("Bob", Sat);

    // Carol: 평일만 5회 (Mon/Tue/Thu/Fri) -> 1점씩 5회 = 5점, 보너스 없음
    sys.addRecord("Carol", Mon);
    sys.addRecord("Carol", Tue);
    sys.addRecord("Carol", Thu);
    sys.addRecord("Carol", Fri);
    sys.addRecord("Carol", Mon);

    sys.compute();
    const std::vector<PlayerStat>& ps = sys.players();
    ASSERT_EQ(3u, ps.size());

    // Alice
    EXPECT_EQ("Alice", ps[0].name);
    EXPECT_EQ(30 + 10, ps[0].totalPoints); // base 30 + bonus 10

    // Bob
    EXPECT_EQ("Bob", ps[1].name);
    EXPECT_EQ(20 + 10, ps[1].totalPoints); // base 20 + bonus 10

    // Carol
    EXPECT_EQ("Carol", ps[2].name);
    EXPECT_EQ(5, ps[2].totalPoints);
}

// grade 결정 테스트
TEST(AttendanceSystemTest, GradeDecisions) {
    CompositeScoringPolicy scoring;
    ThresholdGradePolicy   grade;
    NormalNoWedWeekendElimination elim;
    AttendanceSystem sys(&scoring, &grade, &elim);

    // GOLD: >= 50
    for (int i = 0; i < 17; ++i) sys.addRecord("Goldie", Wed); // 17*3=51 base
    // SILVER: >= 30
    for (int i = 0; i < 10; ++i) sys.addRecord("Silver", Wed); // 30 base + 10 bonus = 40
    // NORMAL: < 30
    for (int i = 0; i < 10; ++i) sys.addRecord("Normalo", Mon); // 10점

    sys.compute();
    const std::vector<PlayerStat>& ps = sys.players();
    ASSERT_EQ(3u, ps.size());

    EXPECT_EQ("GOLD", ps[0].grade);
    EXPECT_EQ("SILVER", ps[1].grade);
    EXPECT_EQ("NORMAL", ps[2].grade);
}

// elimination 후보 결정 테스트
TEST(AttendanceSystemTest, EliminationCandidateRule) {
    CompositeScoringPolicy scoring;
    ThresholdGradePolicy   grade;
    NormalNoWedWeekendElimination elim;
    AttendanceSystem sys(&scoring, &grade, &elim);

    // Normal & 수/주말 한 번도 없음 -> 탈락 후보
    sys.addRecord("Eli", Mon);
    sys.addRecord("Eli", Thu);
    sys.addRecord("Eli", Fri);

    // 수요일 1회 -> 탈락 아님
    sys.addRecord("KeepWed", Wed);

    // 주말 1회 -> 탈락 아님
    sys.addRecord("KeepWeekend", Sat);

    sys.compute();
    const std::vector<PlayerStat>& ps = sys.players();
    ASSERT_EQ(3u, ps.size());

    EXPECT_EQ("Eli", ps[0].name);
    EXPECT_TRUE(ps[0].eliminationCandidate);

    EXPECT_EQ("KeepWed", ps[1].name);
    EXPECT_FALSE(ps[1].eliminationCandidate);

    EXPECT_EQ("KeepWeekend", ps[2].name);
    EXPECT_FALSE(ps[2].eliminationCandidate);
}

// Stream 입력받아서 처리 테스트
TEST(AttendanceSystemTest, LoadFromStream) {
    std::stringstream ss;
    ss << "Umar monday\n"
        << "Daisy tuesday\n"
        << "Alice wednesday\n"
        << "Alice saturday\n"
        << "BadName funday\n"; // invalid 요일은 무시됨

    AttendanceSystem sys; // 기본 정책 자동 장착
    sys.loadFromStream(ss);
    sys.compute();

    const std::vector<PlayerStat>& ps = sys.players();
    ASSERT_EQ(3u, ps.size()); // BadName 무시로 3명만

    // Alice: Wed(3) + Sat(2) = 5점
    EXPECT_EQ("Alice", ps[2].name);
    EXPECT_EQ(5, ps[2].totalPoints);
}

TEST(Extensibility, PlatinumGradeWithoutClientChange) {
    // 80점 이상 PLATINUM 추가
    std::vector<GradeBand> bands;
    GradeBand b;
    b.gradeName = "PLATINUM"; b.minScore = 80; bands.push_back(b);
    b.gradeName = "GOLD"; b.minScore = 50; bands.push_back(b);
    b.gradeName = "SILVER"; b.minScore = 30; bands.push_back(b);
    b.gradeName = "NORMAL"; b.minScore = 0; bands.push_back(b);

    ThresholdGradePolicy platinumPolicy(bands);
    CompositeScoringPolicy scoring;               // 기본
    NormalNoWedWeekendElimination elim;           // 기본

    AttendanceSystem sys(&scoring, &platinumPolicy, &elim); // 주입
    for (int i = 0; i < 27; ++i) sys.addRecord("Ace", Wed); // 27*3=81 → PLATINUM
    sys.compute();

    ASSERT_EQ(1u, sys.players().size());
    EXPECT_EQ("PLATINUM", sys.players()[0].grade);
}
