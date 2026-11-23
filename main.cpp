#include "scheduler.hpp"

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

std::string dayName(int day) {
    const std::string names[] = {
        "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday", "Sunday"
    };

    return names[day - 1];
}

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

void showSchedule(const Scheduler& scheduler) {
    for (const Shift& shift : scheduler.getShifts()) {
        std::cout << "Shift " << shift.getId()
                  << ": " << dayName(shift.getDay())
                  << " " << shift.getStartHour() << ":00-"
                  << shift.getEndHour() << ":00 | ";

        if (!shift.isAssigned()) {
            std::cout << "Unassigned";
        } else {
            for (const Employee& employee : scheduler.getEmployees()) {
                if (employee.getId() == shift.getEmployeeId()) {
                    std::cout << employee.getName();
                    break;
                }
            }
        }

        std::cout << '\n';
    }
}

void runMenu(Scheduler& scheduler) {
    while (true) {
        std::cout << "\n--- Employee Scheduler ---\n"
                  << "1. View employees\n"
                  << "2. View shifts\n"
                  << "3. Find available employees\n"
                  << "4. Assign a shift\n"
                  << "0. Exit\n";

        int choice;

        if (!readNumber("Choose: ", choice) || choice == 0) {
            return;
        }

        try {
            if (choice == 1) {
                for (const Employee& employee : scheduler.getEmployees()) {
                    std::cout << employee.getId() << ": "
                              << employee.getName() << '\n';
                }
            } else if (choice == 2) {
                showSchedule(scheduler);
            } else if (choice == 3) {
                int shiftId;

                if (!readNumber("Shift ID: ", shiftId)) {
                    return;
                }

                std::vector<Employee> available =
                    scheduler.findAvailableEmployees(shiftId);

                if (available.empty()) {
                    std::cout << "No employees available.\n";
                }

                for (const Employee& employee : available) {
                    std::cout << employee.getId() << ": "
                              << employee.getName() << '\n';
                }
            } else if (choice == 4) {
                int shiftId;
                int employeeId;

                if (!readNumber("Shift ID: ", shiftId) ||
                    !readNumber("Employee ID: ", employeeId)) {
                    return;
                }

                std::cout << scheduler.assignShift(employeeId, shiftId);
            } else {
                std::cout << "Please choose an option from 0 to 4.\n";
            }
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << '\n';
        }
    }
}

int main() {
    try {
        Scheduler scheduler(
            "data/employees.json",
            "data/shifts.json"
        );

        runMenu(scheduler);
    } catch (const std::exception& error) {
        std::cerr << "Could not start scheduler: "
                  << error.what() << '\n';
        return 1;
    }

    return 0;
}