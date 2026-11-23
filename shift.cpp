#include "shift.hpp"
#include <stdexcept>

Shift::Shift(int id, int day, int startHour, int endHour)
    : id(id),
      day(day),
      startHour(startHour),
      endHour(endHour),
      employeeId(-1) {

    if (id < 1 || day < 1 || day > 7 ||
        startHour < 0 || endHour > 24 ||
        startHour >= endHour) {
        throw std::invalid_argument("Invalid shift details");
    }
}

int Shift::getId() const {
    return id;
}

int Shift::getDay() const {
    return day;
}

int Shift::getStartHour() const {
    return startHour;
}

int Shift::getEndHour() const {
    return endHour;
}

int Shift::getEmployeeId() const {
    return employeeId;
}

bool Shift::isAssigned() const {
    return employeeId != -1;
}

bool Shift::assignEmployee(int newEmployeeId) {
    if (isAssigned() || newEmployeeId < 1) {
        return false;
    }

    employeeId = newEmployeeId;
    return true;
}

bool Shift::overlaps(const Shift& other) const {
    // Back-to-back shifts do not overlap.
    return day == other.day &&
           startHour < other.endHour &&
           endHour > other.startHour;
}