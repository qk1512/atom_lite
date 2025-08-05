/*
 * app_scheduler.c
 *
 *  Created on: Nov 27, 2019
 *      Modified: Aug 1, 2025
 */
#include "taskSchedule.h"
#include <stdlib.h>

TaskHandle_t TaskScheduler = NULL;

#ifdef __cplusplus
extern "C"
{
#endif
    SemaphoreHandle_t schedulerMutex = NULL;

    typedef struct TaskNode
    {
        void (*pTask)(void);
        uint32_t Delay;
        uint32_t Period;
        uint8_t RunMe;
        uint32_t TaskID;
        struct TaskNode *next;
    } TaskNode;

    // Pointer to the head of the linked list
    static TaskNode *head = NULL;
    static uint32_t newTaskID = 0;
    static uint32_t count_SCH_Update = 0;
    static uint32_t taskCount = 0;

    static uint32_t Get_New_Task_ID(void);

    void SCH_Init(void)
    {
        // Create mutex for thread safety
        if (schedulerMutex == NULL)
        {
            schedulerMutex = xSemaphoreCreateMutex();
            if (schedulerMutex == NULL)
            {
                Serial.println("ERROR: Failed to create scheduler mutex");
                return;
            }
        }

        if (xSemaphoreTake(schedulerMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            head = NULL; // Initialize the linked list as empty
            newTaskID = 0;
            count_SCH_Update = 0;
            taskCount = 0;
            xSemaphoreGive(schedulerMutex);
        }

        Serial.println("Scheduler initialized successfully");
    }

    void SCH_Update(void)
    {
        if (xSemaphoreTake(schedulerMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            count_SCH_Update++;

            if (head != NULL && head->RunMe == 0)
            {
                if (head->Delay > 0)
                {
                    head->Delay = head->Delay - 1;
                }
                if (head->Delay == 0)
                {
                    head->RunMe = 1;
                }
            }

            xSemaphoreGive(schedulerMutex);
        }
    }

    uint32_t SCH_Add_Task(void (*pFunction)(), uint32_t DELAY, uint32_t PERIOD)
    {
        if (pFunction == NULL)
        {
            Serial.println("ERROR: NULL function pointer in SCH_Add_Task");
            return NO_TASK_ID;
        }

        if (taskCount >= SCH_MAX_TASKS)
        {
            Serial.println("ERROR: Maximum number of tasks reached");
            return NO_TASK_ID;
        }

        TaskNode *newTask = (TaskNode *)malloc(sizeof(TaskNode));
        if (newTask == NULL)
        {
            Serial.println("ERROR: Memory allocation failed in SCH_Add_Task");
            return NO_TASK_ID;
        }

        newTask->pTask = pFunction;
        newTask->Period = PERIOD;
        newTask->TaskID = Get_New_Task_ID();
        newTask->RunMe = 0;

        if (xSemaphoreTake(schedulerMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // If the list is empty or the new task should be inserted at the beginning
            if (head == NULL || DELAY < head->Delay)
            {
                if (head != NULL)
                {
                    // Adjust the delay of the current head
                    head->Delay = head->Delay - DELAY;
                }
                newTask->Delay = DELAY;
                newTask->next = head;
                head = newTask;

                if (newTask->Delay == 0)
                {
                    newTask->RunMe = 1;
                }

                taskCount++;

                Serial.print("Added task ID ");
                Serial.print(newTask->TaskID);
                Serial.print(" at head with delay ");
                Serial.println(newTask->Delay);

                xSemaphoreGive(schedulerMutex);
                return newTask->TaskID;
            }

            // Find the correct position in the linked list
            TaskNode *current = head;
            uint32_t sumDelay = current->Delay;

            while (current->next != NULL && sumDelay + current->next->Delay <= DELAY)
            {
                current = current->next;
                sumDelay += current->Delay;
            }

            // Insert the new task
            newTask->Delay = DELAY - sumDelay;

            if (current->next != NULL)
            {
                // Adjust the delay of the next task
                current->next->Delay = current->next->Delay - newTask->Delay;
            }

            newTask->next = current->next;
            current->next = newTask;

            if (newTask->Delay == 0)
            {
                newTask->RunMe = 1;
            }

            taskCount++;

            Serial.print("Added task ID ");
            Serial.print(newTask->TaskID);
            Serial.print(" after task ID ");
            Serial.print(current->TaskID);
            Serial.print(" with delay ");
            Serial.println(newTask->Delay);

            xSemaphoreGive(schedulerMutex);
            return newTask->TaskID;
        }

        // If we couldn't get the mutex, free the allocated memory
        free(newTask);
        Serial.println("ERROR: Could not acquire mutex in SCH_Add_Task");
        return NO_TASK_ID;
    }

    uint8_t SCH_Delete_Task(uint32_t taskID)
    {
        uint8_t Return_code = 0;

        if (taskID == NO_TASK_ID)
        {
            return Return_code;
        }

        if (xSemaphoreTake(schedulerMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // Special case for head
            if (head != NULL && head->TaskID == taskID)
            {
                TaskNode *temp = head;

                // If there's a next task, add the current head's delay to it
                if (head->next != NULL)
                {
                    head->next->Delay += head->Delay;
                }

                head = head->next;
                taskCount--;

                Serial.print("Deleted task ID ");
                Serial.print(temp->TaskID);
                Serial.println(" from head");

                free(temp);
                Return_code = 1;
            }
            else if (head != NULL)
            {
                // Search through the list
                TaskNode *current = head;

                while (current->next != NULL && current->next->TaskID != taskID)
                {
                    current = current->next;
                }

                if (current->next != NULL)
                {
                    // Found the task to delete
                    TaskNode *temp = current->next;

                    // Add the delay to the next task if it exists
                    if (temp->next != NULL)
                    {
                        temp->next->Delay += temp->Delay;
                    }

                    current->next = temp->next;
                    taskCount--;

                    Serial.print("Deleted task ID ");
                    Serial.print(temp->TaskID);
                    Serial.print(" after task ID ");
                    Serial.println(current->TaskID);

                    free(temp);
                    Return_code = 1;
                }
            }

            xSemaphoreGive(schedulerMutex);
        }

        return Return_code;
    }

    void SCH_Dispatch_Tasks(void)
    {
        if (xSemaphoreTake(schedulerMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            if (head != NULL && head->RunMe > 0)
            {
                void (*pTask)() = head->pTask;
                uint32_t Period = head->Period;
                uint32_t TaskID = head->TaskID;

                Serial.print("Dispatching task ID ");
                Serial.println(TaskID);

                // Release mutex before executing task to prevent deadlock
                xSemaphoreGive(schedulerMutex);

                // Execute the task (outside mutex to prevent blocking other operations)
                if (pTask != NULL)
                {
                    (*pTask)();
                }
                // Delete the task - it's always at the head
                SCH_Delete_Task(TaskID);
                if (Period != 0)
                {
                    Serial.print("Re-adding periodic task ID ");
                    Serial.print(TaskID);
                    Serial.print(" with period ");
                    Serial.println(Period);

                    // Release mutex before adding to prevent potential issues
                    SCH_Add_Task(pTask, Period, Period);
                }
            }
            else
            {
                xSemaphoreGive(schedulerMutex);
            }
        }
    }

    // Function to print all tasks in the list
    void SCH_Print_Tasks(void)
    {
        if (xSemaphoreTake(schedulerMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            Serial.println("\n--- Task List ---");
            Serial.print("Total tasks: ");
            Serial.println(taskCount);

            if (head == NULL)
            {
                Serial.println("List is empty");
            }
            else
            {
                TaskNode *current = head;
                int position = 0;

                while (current != NULL)
                {
                    Serial.print("Position: ");
                    Serial.print(position);
                    Serial.print(", Task ID: ");
                    Serial.print(current->TaskID);
                    Serial.print(", Delay: ");
                    Serial.print(current->Delay);
                    Serial.print(", Period: ");
                    Serial.print(current->Period);
                    Serial.print(", RunMe: ");
                    Serial.println(current->RunMe);

                    current = current->next;
                    position++;
                }
            }
            Serial.println("----------------\n");

            xSemaphoreGive(schedulerMutex);
        }
    }

    uint32_t SCH_Get_Task_Count(void)
    {
        uint32_t count = 0;
        if (xSemaphoreTake(schedulerMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            count = taskCount;
            xSemaphoreGive(schedulerMutex);
        }
        return count;
    }

    static uint32_t Get_New_Task_ID(void)
    {
        newTaskID++;
        if (newTaskID == NO_TASK_ID)
        {
            newTaskID++;
        }
        return newTaskID;
    }

    void TaskSchedule(void *pvParameters)
    {
        TickType_t xLastWakeTime = xTaskGetTickCount();

        Serial.println("Scheduler task started");

        while (true)
        {
            // Update scheduler timing
            SCH_Update();

            // Dispatch ready tasks
            SCH_Dispatch_Tasks();

            // Print tasks
            SCH_Print_Tasks();

            // Maintain precise timing
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // Cleanup function (call before system reset)
    void SCH_Cleanup(void)
    {
        if (xSemaphoreTake(schedulerMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // Free all tasks in the list
            TaskNode *current = head;
            while (current != NULL)
            {
                TaskNode *temp = current;
                current = current->next;
                free(temp);
            }
            head = NULL;
            taskCount = 0;

            xSemaphoreGive(schedulerMutex);
        }

        // Delete scheduler task
        if (TaskScheduler != NULL)
        {
            vTaskDelete(TaskScheduler);
            TaskScheduler = NULL;
        }

        // Delete mutex
        if (schedulerMutex != NULL)
        {
            vSemaphoreDelete(schedulerMutex);
            schedulerMutex = NULL;
        }

        Serial.println("Scheduler cleanup completed");
    }

#ifdef __cplusplus
}
#endif

void initSetUpScheduler()
{
    // Initialize scheduler
    SCH_Init();

    // Create scheduler task with higher stack size
    BaseType_t result = xTaskCreate(
        TaskSchedule,
        "SchedulerTask",
        4096, // Increased stack size
        NULL,
        2, // Higher priority than other tasks
        &TaskScheduler);

    if (result == pdPASS)
    {
        Serial.println("Scheduler task created successfully");
    }
    else
    {
        Serial.println("ERROR: Failed to create scheduler task");
    }
}