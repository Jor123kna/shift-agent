#include "scheduler.hpp"
#include "storage.hpp"

#include <stdexcept>

Scheduler::Scheduler(const std::string& employeesFile,
                     const std::string& shiftsFile)
    : shiftsFile(shiftsFile) {

    employees = loadEmployees(employeesFile);
    shifts = loadShifts(shiftsFile, employees);
}

const std::vector<Employee>& Scheduler::getEmployees() const {
    return employees;
}

const std::vector<Shift>& Scheduler::getShifts() const {
    return shifts;
}

bool Scheduler::canAssign(const Employee& employee,
                          const Shift& shift) const {
    if (shift.isAssigned()) {
        return false;
    }

    if (!employee.isAvailable(
            shift.getDay(),
            shift.getStartHour(),
            shift.getEndHour())) {
        return false;
    }

    for (const Shift& existing : shifts) {
        if (existing.getEmployeeId() == employee.getId() &&
            existing.overlaps(shift)) {
            return false;
        }
    }

    return true;
}

std::vector<Employee> Scheduler::findAvailableEmployees(
    int shiftId
) const {
    const Shift* selected = nullptr;

    for (const Shift& shift : shifts) {
        if (shift.getId() == shiftId) {
            selected = &shift;
            break;
        }
    }

    if (selected == nullptr) {
        throw std::runtime_error("Shift not found.");
    }

    if (selected->isAssigned()) {
        throw std::runtime_error("This shift is already assigned.");
    }

    std::vector<Employee> available;

    for (const Employee& employee : employees) {
        if (canAssign(employee, *selected)) {
            available.push_back(employee);
        }
    }

    return available;
}

std::string Scheduler::assignShift(int employeeId, int shiftId) {
    const Employee* selectedEmployee = nullptr;
    const Shift* selectedShift = nullptr;

    for (const Employee& employee : employees) {
        if (employee.getId() == employeeId) {
            selectedEmployee = &employee;
            break;
        }
    }

    for (const Shift& shift : shifts) {
        if (shift.getId() == shiftId) {
            selectedShift = &shift;
            break;
        }
    }

    if (selectedEmployee == nullptr) {
        throw std::runtime_error("Employee not found.");
    }

    if (selectedShift == nullptr) {
        throw std::runtime_error("Shift not found.");
    }

    if (selectedShift->isAssigned()) {
        throw std::runtime_error("This shift is already assigned.");
    }

    if (!canAssign(*selectedEmployee, *selectedShift)) {
        throw std::runtime_error(
            "Employee is unavailable or has an overlapping shift."
        );
    }

    std::vector<Shift> updatedShifts = shifts;

    for (Shift& shift : updatedShifts) {
        if (shift.getId() == shiftId) {
            if (!shift.assignEmployee(employeeId)) {
                throw std::runtime_error("Could not assign shift.");
            }
            break;
        }
    }

    // Prepare the message before replacing the shifts vector.
    const std::string days[] = {
        "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday", "Sunday"
    };

    std::string message =
        "Shift assigned and saved.\n\n"
        "[Simulated email]\nHello " +
        selectedEmployee->getName() +
        ", you are scheduled on " +
        days[selectedShift->getDay() - 1] +
        " from " +
        std::to_string(selectedShift->getStartHour()) +
        ":00 to " +
        std::to_string(selectedShift->getEndHour()) +
        ":00.\n";

    // If saving fails, the original in-memory schedule stays unchanged.
    saveShifts(shiftsFile, updatedShifts);
    shifts = updatedShifts;

    return message;
}