<img width="272" height="176" alt="image" src="https://github.com/user-attachments/assets/57ec9f71-4116-4feb-8e6c-558f52ee8269" />

<img width="1920" height="564" alt="image" src="https://github.com/user-attachments/assets/60c1d3ad-b4ff-4dc2-8199-9edcb1a54186" />

<img width="1920" height="896" alt="image" src="https://github.com/user-attachments/assets/2d679f56-e1e4-49db-8046-c42ea99edca8" />



1. 함수 레벨 리팩토링

   D1 - ScoringPolicy, GradeBand, PlayerStat, Record 등 역할별로 분리했고 의미에 맞는 함수명 사용함.


2. 클래스 레벨 리팩토링

   D2 - GTest 활용해서 UT 생성함. 요일, Id할당, 스코어 계산, Grade결정, Elimination 결정 테스트 등 추가함.

   D3 – IScoringPolicy, IGradePolicy, IEliminationRule 등의 인터페이스와 DefaultPolicyFactory로 정책을 생성해 주입 -> 새 정책 / 등급 추가 시 클라이언트 코드 변경 없도록 함


3. 디자인 패턴 사용하기

   D4 - Strategy(정책 교체), Factory(정책 세트 생성), Facade(간단한 입출력 API로 내부 복잡도 은닉) 적용함.


4. 코드 커버리지 100%

   D5 - UT 보강(Factory 경로, 예외처리(UNDEFINED 분기, File 입출력 실패 등), Print관련)으로 커버리지 100% 달성완료.
