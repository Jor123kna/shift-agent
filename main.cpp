#include "employee.hpp"
#include "shift.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string dayName(int day) {
    const std::string names[] = {
        "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday", "Sunday"
    };

    return names[day - 1];
}

// Reject invalid input, such as letters or "12abc".
bool readNumber(const std::string& prompt, int& number) {
    std::string line;

    while (true) {
        std::cout << prompt;

        if (!std::getline(std::cin, line)) {
            return false;
        }

        std::istringstream input(line);
        char extra;

        if ((input >> number) && !(input >> extra)) {
            return true;
        }

        std::cout << "Please enter a whole number.\n";
    }
}

bool canAssign(const Employee& employee,
               const Shift& requested,
               const std::vector<Shift>& shifts) {

    if (requested.isAssigned()) {
        return false;
    }

    if (!employee.isAvailable(requested.getDay(),
                              requested.getStartHour(),
                              requested.getEndHour())) {
        return false;
    }

    for (const Shift& existing : shifts) {
        if (existing.getEmployeeId() == employee.getId() &&
            existing.overlaps(requested)) {
            return false;
        }
    }

    return true;
}

void showSchedule(const std::vector<Shift>& shifts,
                  const std::vector<Employee>& employees) {

    for (const Shift& shift : shifts) {
        std::cout << "Shift " << shift.getId()
                  << ": " << dayName(shift.getDay())
                  << " " << shift.getStartHour() << ":00-"
                  << shift.getEndHour() << ":00 | ";

        if (!shift.isAssigned()) {
            std::cout << "Unassigned";
        } else {
            for (const Employee& employee : employees) {
                if (employee.getId() == shift.getEmployeeId()) {
                    std::cout << employee.getName();
                }
            }
        }

        std::cout << '\n';
    }
}

int main() {
    std::vector<Employee> employees = {
        Employee(1, "Alex"),
        Employee(2, "Sam"),
        Employee(3, "Taylor")
    };

    // Example availability for Friday.
    employees[0].addAvailability(5, 9, 17);
    employees[1].addAvailability(5, 12, 20);
    employees[2].addAvailability(5, 8, 16);

    std::vector<Shift> shifts = {
        Shift(1, 5, 9, 13),
        Shift(2, 5, 12, 16),
        Shift(3, 5, 16, 20)
    };

    while (true) {
        std::cout << "\n--- Employee Scheduler ---\n"
                  << "1. View employees\n"
                  << "2. View shifts\n"
                  << "3. Find available employees\n"
                  << "4. Assign a shift\n"
                  << "0. Exit\n";

        int choice;

        if (!readNumber("Choose: ", choice) || choice == 0) {
            break;
        }

        if (choice == 1) {
            for (const Employee& employee : employees) {
                std::cout << employee.getId() << ": "
                          << employee.getName() << '\n';
            }
        } else if (choice == 2) {
            showSchedule(shifts, employees);
        } else if (choice == 3 || choice == 4) {
            int shiftId;

            if (!readNumber("Shift ID: ", shiftId)) {
                break;
            }

            Shift* selected = nullptr;

            for (Shift& shift : shifts) {
                if (shift.getId() == shiftId) {
                    selected = &shift;
                }
            }

            if (selected == nullptr) {
                std::cout << "Shift not found.\n";
                continue;
            }

            if (selected->isAssigned()) {
                std::cout << "This shift is already assigned.\n";
                continue;
            }

            if (choice == 3) {
                bool found = false;

                for (const Employee& employee : employees) {
                    if (canAssign(employee, *selected, shifts)) {
                        std::cout << employee.getId() << ": "
                                  << employee.getName() << '\n';
                        found = true;
                    }
                }

                if (!found) {
                    std::cout << "No employees available.\n";
                }
            } else {
                int employeeId;

                if (!readNumber("Employee ID: ", employeeId)) {
                    break;
                }

                const Employee* selectedEmployee = nullptr;

                for (const Employee& employee : employees) {
                    if (employee.getId() == employeeId) {
                        selectedEmployee = &employee;
                    }
                }

                if (selectedEmployee == nullptr) {
                    std::cout << "Employee not found.\n";
                } else if (!canAssign(*selectedEmployee,
                                      *selected, shifts)) {
                    std::cout
                        << "Cannot assign: unavailable or overlapping shift.\n";
                } else if (selected->assignEmployee(employeeId)) {
                    std::cout << "Shift assigned.\n";

                    std::cout << "\n[Simulated email]\n"
                              << "Hello "
                              << selectedEmployee->getName()
                              << ", you are scheduled on "
                              << dayName(selected->getDay())
                              << " from "
                              << selected->getStartHour() << ":00 to "
                              << selected->getEndHour() << ":00.\n";
                }
            }
        } else {
            std::cout << "Please choose an option from 0 to 4.\n";
        }
    }

    return 0;
}