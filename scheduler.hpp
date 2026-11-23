#pragma once

#include "employee.hpp"
#include "shift.hpp"

#include <string>
#include <vector>

class Scheduler {
private:
    std::vector<Employee> employees;
    std::vector<Shift> shifts;
    std::string shiftsFile;

    bool canAssign(const Employee& employee, const Shift& shift) const;

public:
    Scheduler(const std::string& employeesFile,
              const std::string& shiftsFile);

    const std::vector<Employee>& getEmployees() const;
    const std::vector<Shift>& getShifts() const;

    std::vector<Employee> findAvailableEmployees(int shiftId) const;

    // Returns a success message or throws an exception.
    std::string assignShift(int employeeId, int shiftId);
};

