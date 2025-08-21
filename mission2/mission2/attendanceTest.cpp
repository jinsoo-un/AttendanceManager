#include "attendance.h"
#include <gtest/gtest.h>
#include <sstream>

// ���� �Ľ� �׽�Ʈ
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

// ID �Ҵ� ���� �׽�Ʈ
TEST(AttendanceSystemTest, IdAssignmentOrder) {
    AttendanceSystem sys;
    sys.addRecord("Alice", Mon);    // id=1
    sys.addRecord("Bob", Tue);      // id=2
    sys.addRecord("Alice", Wed);    // id=1
    sys.compute();

    const std::vector<PlayerStat>& ps = sys.players();
    ASSERT_EQ(2u, ps.size());
    EXPECT_EQ(1, ps[0].id);
    EXPECT_EQ("Alice", ps[0].name);
    EXPECT_EQ(2, ps[0].dayCount[Mon] + ps[0].dayCount[Wed]); // just sanity
    EXPECT_EQ(2, ps[1].id);
    EXPECT_EQ("Bob", ps[1].name);
}

// ���ھ� ��� �׽�Ʈ
TEST(AttendanceSystemTest, BaseAndBonus) {
    ScoringPolicy sp; GradePolicy gp;
    AttendanceSystem sys(sp, gp);

    // Alice: Wed 10ȸ -> ������ ���ʽ� +10, �� ������ �⺻���� 3*10 = 30
    for (int i = 0; i < 10; ++i) sys.addRecord("Alice", Wed);

    // Bob: Sat 10ȸ -> �ָ� ���ʽ� +10, �� ����� �⺻���� 2*10 = 20
    for (int i = 0; i < 10; ++i) sys.addRecord("Bob", Sat);

    // Carol: ���ϸ� 5ȸ (Mon/Tue/Thu/Fri) -> 1���� 5ȸ = 5��, ���ʽ� ����
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

// grade ���� �׽�Ʈ
TEST(AttendanceSystemTest, GradeDecisions) {
    ScoringPolicy sp; GradePolicy gp;
    AttendanceSystem sys(sp, gp);

    // GOLD: >= 50
    for (int i = 0; i < 17; ++i) sys.addRecord("Goldie", Wed); // 17*3=51 base, ���ʽ��� ���� �� ����
    // SILVER: >= 30
    for (int i = 0; i < 10; ++i) sys.addRecord("Silver", Wed); // 30 base + 10 bonus = 40 (Ȯ���� �ǹ� �̻�)
    // NORMAL: < 30
    for (int i = 0; i < 10; ++i) sys.addRecord("Normalo", Mon); // 10��

    sys.compute();
    const std::vector<PlayerStat>& ps = sys.players();
    ASSERT_EQ(3u, ps.size());

    EXPECT_EQ("GOLD", ps[0].grade);
    EXPECT_EQ("SILVER", ps[1].grade);
    EXPECT_EQ("NORMAL", ps[2].grade);
}

// elimination �ĺ� ���� �׽�Ʈ
TEST(AttendanceSystemTest, EliminationCandidateRule) {
    ScoringPolicy sp; GradePolicy gp;
    AttendanceSystem sys(sp, gp);

    // Normal & ��/�ָ� �� ���� ���� -> Ż�� �ĺ�
    sys.addRecord("Eli", Mon);
    sys.addRecord("Eli", Thu);
    sys.addRecord("Eli", Fri);

    // ������ 1ȸ -> Ż�� �ƴ�
    sys.addRecord("KeepWed", Wed);

    // �ָ� 1ȸ -> Ż�� �ƴ�
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

// Stream �Է¹޾Ƽ� ó�� �׽�Ʈ
TEST(AttendanceSystemTest, LoadFromStream) {
    std::stringstream ss;
    ss << "Umar monday\n"
        << "Daisy tuesday\n"
        << "Alice wednesday\n"
        << "Alice saturday\n"
		<< "BadName funday\n"; // invalid ������ ���õ�

    AttendanceSystem sys;
    sys.loadFromStream(ss);
    sys.compute();

    const std::vector<PlayerStat>& ps = sys.players();
    ASSERT_EQ(3u, ps.size()); // BadName ���÷� 3��

    // Alice: Wed(3) + Sat(2) = 5��
    EXPECT_EQ("Alice", ps[2].name);
    EXPECT_EQ(5, ps[2].totalPoints);
}
