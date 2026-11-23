#include "employee.hpp"

Employee::Employee(int id, const std::string& name)
    : id(id), name(name) {
}

int Employee::getId() const {
    return id;
}

const std::string& Employee::getName() const {
    return name;
}

bool Employee::addAvailability(int day, int startHour, int endHour) {
    if (day < 1 || day > 7 || startHour < 0 ||
        endHour > 24 || startHour >= endHour) {
        return false;
    }

    availability.push_back({day, startHour, endHour});
    return true;
}

bool Employee::isAvailable(int day, int startHour, int endHour) const {
    if (startHour < 0 || endHour > 24 || startHour >= endHour) {
        return false;
    }

    for (const Availability& time : availability) {
        if (time.day == day &&
            startHour >= time.startHour &&
            endHour <= time.endHour) {
            return true;
        }
    }

    return false;
}