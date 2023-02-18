# Students Assistance

This project mimics the flow when one assistance serves several students. 
Attendance only has 3 seats for students awaiting care and 1 seat for the student receiving care. 
If there are no seats available, students will return later (students are threaded). 
The attendant can attend to only one student at a time and once he is done with one, 
he can attend to the next student and a waiting seat will be freed up or they can go to sleep and come back later. 
Since each student is a thread, the program uses locks to avoid race conditions.

## Requirements

- [GCC Compiler](https://gcc.gnu.org/)

## How to install & use

1. Compile the main.c
```
gcc main.c
```

2. Run the output from last command and pass the number of students that you want to create through arguments
```
main.exe 12 // this is the number of students
```
