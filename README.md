# Computer Architecture Project 

### Project 1

MIPS(Big-endian) 어셈블리 코드를 바이너리 코드로 변환하는 MIPS ISA assembler 구현.

![image](https://github.com/user-attachments/assets/0556ba78-f59e-47e0-a191-1cc2e2862ded)

위와 같은 assembly file (*.s)을 input 으로 받고, input 파일을 바이너리 파일로 변환한 object file (*.o)을 output 파일로 출력

### Project 2

MIPS 인스트럭션을 실행시킬 수 있는 간단한 MIPS 에뮬레이터를 구현. 앞서 진행된 첫 번째 과제가 어셈블리 코드를 바이너리로 바꾸는 작업이었다면, 이번 과제의 목표는 생성된 바이너리를 입력 받아 메모리에 로드하고, 인스트럭션을 실행시키는 에뮬레이터를 구현하는 것. 인스트럭션이 수행됨에 따라 레지스터와 메모리의 상태가 변함.

### Project 3

두 번째 과제에서 구현하였던 에뮬레이터의 확장으로, MIPS ISA를 대상으로 다섯 단계의 파이프라인을 구현. 이때 파이프라인 과정에서 발생하는 해저드(hazard)를 막고 전방전달(forwarding)을 구현하기 위해 각 파이프라인 단계 사이에 파이프라인 상태 레지스터(pipelining state register)등을 구현해야 함.

### Project 4

2단계(Two level) 캐시 시뮬레이터 구현 및 실험을 통해 캐시 구조 이해.

#### Result
![image](https://github.com/user-attachments/assets/8950074c-0cbb-46a3-8628-e9350ec2ed40)

#### Environment
Ubuntu 20.04 / GCC 9.4.0
