#pragma once

class Shift {
private:
    int id;
    int day;
    int startHour;
    int endHour;
    int employeeId; 

public:
    Shift(int id, int day, int startHour, int endHour);

    int getId() const;
    int getDay() const;
    int getStartHour() const;
    int getEndHour() const;
    int getEmployeeId() const;

    bool isAssigned() const;
    bool assignEmployee(int newEmployeeId);
    bool overlaps(const Shift& other) const;
};

