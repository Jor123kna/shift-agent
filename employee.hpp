#pragma once

#include <string>
#include <vector>

struct Availability {
    int day; // Monday = 1, Sunday = 7
    int startHour;
    int endHour;
};

class Employee {
private:
    int id;
    std::string name;
    std::vector<Availability> availability;

public:
    Employee(int id, const std::string& name);

    int getId() const;
    const std::string& getName() const;

    bool addAvailability(int day, int startHour, int endHour);
    bool isAvailable(int day, int startHour, int endHour) const;
};