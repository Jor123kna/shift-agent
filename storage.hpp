#pragma once

#include "employee.hpp"
#include "shift.hpp"

#include <string>
#include <vector>

std::vector<Employee> loadEmployees(const std::string& filename);

std::vector<Shift> loadShifts(
    const std::string& filename,
    const std::vector<Employee>& employees
);

void saveShifts(
    const std::string& filename,
    const std::vector<Shift>& shifts
);

