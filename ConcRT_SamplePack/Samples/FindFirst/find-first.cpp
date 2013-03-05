// find-first.cpp
// compile with: /EHsc
#include <windows.h>
#include <agents.h>
#include <ppl.h>
#include <array>

using namespace Concurrency;
using namespace std;
using namespace std::tr1;

// Contains information about an employee.
struct employee
{
   int id;
   double salary;
};

// Finds the first employee that has the provided id or salary.
template <typename T>
void find_employee(const T& employees, int id, double salary)
{
   // Holds the salary for the employee with the provided id.
   single_assignment<double> find_id_result;

   // Holds the id for the employee with the provided salary.
   single_assignment<int> find_salary_result;


   // Holds a message if no employee with the provided id exists.
   single_assignment<bool> id_not_found;

   // Holds a message if no employee with the provided salary exists.
   single_assignment<bool> salary_not_found;

   // Create a join object for the "not found" buffers.
   // This join object sends a message when both its members holds a message 
   // (in other words, no employee with the provided id or salary exists).
   auto not_found = make_join(&id_not_found, &salary_not_found);


   // Create a choice object to select among the following cases:
   // 1. An employee with the provided id exists.
   // 2. An employee with the provided salary exists.
   // 3. No employee with the provided id or salary exists.
   auto selector = make_choice(&find_id_result, &find_salary_result, &not_found);
   

   // Create a task that searches for the employee with the provided id.
   auto search_id_task = make_task([&]{
      auto result = find_if(employees.begin(), employees.end(), 
         [&](const employee& e) { return e.id == id; });
      if (result != employees.end())
      {
         // The id was found, send the salary to the result buffer.
         send(find_id_result, result->salary);
      }
      else
      {
         // The id was not found.
         send(id_not_found, true);
      }
   });

   // Create a task that searches for the employee with the provided salary.
   auto search_salary_task = make_task([&]{
      auto result = find_if(employees.begin(), employees.end(), 
         [&](const employee& e) { return e.salary == salary; });
      if (result != employees.end())
      {
         // The salary was found, send the id to the result buffer.
         send(find_salary_result, result->id);
      }
      else
      {
         // The salary was not found.
         send(salary_not_found, true);
      }
   });

   // Use a structured_task_group object to run both tasks.
   structured_task_group tasks;
   tasks.run(search_id_task);
   tasks.run(search_salary_task);

   // Receive the first object that holds a message and print a message.
   size_t index = receive(selector);
   switch (index)
   {
   case 0:
      printf_s("Employee with id %d has salary %.2f.\n", id, receive(find_id_result));
      break;
   case 1:
      printf_s("Employee with salary %.2f has id %d.\n", salary, receive(find_salary_result));
      break;
   case 2:
      printf_s("No employee has id %d or salary %.2f.\n", id, salary);
      break;
   default:
      __assume(0);
   }
   
   // Cancel any active tasks and wait for the task group to finish.
   tasks.cancel();
   tasks.wait();
}

int main()
{
   srand(15);
   
   // Create an array of employees and assign each one a 
   // random id and salary.

   array<employee, 10000> employees;
   
   const double base_salary = 25000.0f;
   for (size_t i = 0; i < employees.size(); ++i)
   {
      employees[i].id = rand();

      double bonus = static_cast<double>(rand()) / 100.0f;
      employees[i].salary = base_salary + bonus;
   }

   // Search for several id and salary values.

   find_employee(employees, 698, 30210.00);
   find_employee(employees, 340, 25079.59);
   find_employee(employees, 27100, 29255.90);
   find_employee(employees, 899, 31223.45);
}