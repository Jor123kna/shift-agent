#include "storage.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>

using json = nlohmann::json;

json readJson(const std::string& filename) {
    std::ifstream file(filename);

    if (!file) {
        throw std::runtime_error("Cannot open " + filename);
    }

    json data;
    file >> data;

    if (!data.is_array()) {
        throw std::runtime_error(
            filename + " must contain a JSON array."
        );
    }

    return data;
}

std::vector<Employee> loadEmployees(const std::string& filename) {
    json data = readJson(filename);
    std::vector<Employee> employees;

    for (const auto& item : data) {
        int id = item.at("id").get<int>();

        if (id < 1) {
            throw std::runtime_error("Employee IDs must be positive.");
        }

        for (const Employee& existing : employees) {
            if (existing.getId() == id) {
                throw std::runtime_error("Duplicate employee ID.");
            }
        }

        Employee employee(id, item.at("name").get<std::string>());

        const auto& availability = item.at("availability");

        if (!availability.is_array()) {
            throw std::runtime_error("Availability must be an array.");
        }

        for (const auto& time : availability) {
            bool valid = employee.addAvailability(
                time.at("day").get<int>(),
                time.at("startHour").get<int>(),
                time.at("endHour").get<int>()
            );

            if (!valid) {
                throw std::runtime_error("Invalid availability.");
            }
        }

        employees.push_back(employee);
    }

    return employees;
}

std::vector<Shift> loadShifts(
    const std::string& filename,
    const std::vector<Employee>& employees
) {
    json data = readJson(filename);
    std::vector<Shift> shifts;

    for (const auto& item : data) {
        Shift shift(
            item.at("id").get<int>(),
            item.at("day").get<int>(),
            item.at("startHour").get<int>(),
            item.at("endHour").get<int>()
        );

        for (const Shift& existing : shifts) {
            if (existing.getId() == shift.getId()) {
                throw std::runtime_error("Duplicate shift ID.");
            }
        }

        int employeeId = item.at("employeeId").get<int>();

        if (employeeId != -1) {
            const Employee* assignedEmployee = nullptr;

            for (const Employee& employee : employees) {
                if (employee.getId() == employeeId) {
                    assignedEmployee = &employee;
                }
            }

            if (assignedEmployee == nullptr) {
                throw std::runtime_error(
                    "Shift refers to an unknown employee."
                );
            }

            if (!assignedEmployee->isAvailable(
                    shift.getDay(),
                    shift.getStartHour(),
                    shift.getEndHour())) {
                throw std::runtime_error(
                    "Saved shift is outside employee availability."
                );
            }

            for (const Shift& existing : shifts) {
                if (existing.getEmployeeId() == employeeId &&
                    existing.overlaps(shift)) {
                    throw std::runtime_error(
                        "Saved shifts overlap for an employee."
                    );
                }
            }

            if (!shift.assignEmployee(employeeId)) {
                throw std::runtime_error("Invalid saved assignment.");
            }
        }

        shifts.push_back(shift);
    }

    return shifts;
}

void saveShifts(
    const std::string& filename,
    const std::vector<Shift>& shifts
) {
    json data = json::array();

    for (const Shift& shift : shifts) {
        data.push_back({
            {"id", shift.getId()},
            {"day", shift.getDay()},
            {"startHour", shift.getStartHour()},
            {"endHour", shift.getEndHour()},
            {"employeeId", shift.getEmployeeId()}
        });
    }

    std::string temporaryFile = filename + ".tmp";

    std::ofstream file(temporaryFile);

    if (!file) {
        throw std::runtime_error("Cannot write " + temporaryFile);
    }

    file << std::setw(4) << data << '\n';
    file.close();

    if (!file) {
        throw std::runtime_error("Failed to save " + temporaryFile);
    }

    std::filesystem::rename(temporaryFile, filename);
}