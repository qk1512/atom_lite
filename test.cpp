/*
 * app_scheduler.c
 *
 *  Created on: Nov 27, 2019
 *      Modified: Apr 7, 2025
 */
#include "Arduino.h"
#include "taskSchedule.h"
#include <stdlib.h> // For malloc and free

#ifdef __cplusplus
extern "C"
{
#endif

    TaskHandle_t TaskScheduler = NULL;

#define INTERRUPT_CYCLE 10 // 10 milliseconds
#define PRESCALER 64
#define COUNTER_START 65536 - INTERRUPT_CYCLE * 1000 * 16 / 64 // 16M is Core Clock

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

    static uint32_t Get_New_Task_ID(void);
    // static void TIMER_Init();
    void SCH_Print_Tasks(void);

    void SCH_Init(void)
    {
        head = NULL; // Initialize the linked list as empty
                     // TIMER_Init();
    }

    void SCH_Update(void)
    {
        // Check if there is a task at the head of the list
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
    }

    uint32_t SCH_Add_Task(void (*pFunction)(), uint32_t DELAY, uint32_t PERIOD)
    {
        TaskNode *newTask = (TaskNode *)malloc(sizeof(TaskNode));
        if (newTask == NULL)
        {
            // Handle memory allocation failure
            Serial.println("ERROR: Memory allocation failed in SCH_Add_Task");
            return NO_TASK_ID;
        }

        newTask->pTask = pFunction;
        newTask->Period = PERIOD;
        newTask->TaskID = Get_New_Task_ID();

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
            else
            {
                newTask->RunMe = 0;
            }

            Serial.print("Added task ID ");
            Serial.print(newTask->TaskID);
            Serial.print(" at head with delay ");
            Serial.println(newTask->Delay);

            SCH_Print_Tasks(); // Print the task list after adding
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
        else
        {
            newTask->RunMe = 0;
        }

        Serial.print("Added task ID ");
        Serial.print(newTask->TaskID);
        Serial.print(" after task ID ");
        Serial.print(current->TaskID);
        Serial.print(" with delay ");
        Serial.println(newTask->Delay);

        SCH_Print_Tasks(); // Print the task list after adding
        return newTask->TaskID;
    }

    uint8_t SCH_Delete_Task(uint32_t taskID)
    {
        uint8_t Return_code = 0;

        if (taskID != NO_TASK_ID)
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

                    Serial.print("Deleted task ID ");
                    Serial.print(temp->TaskID);
                    Serial.print(" after task ID ");
                    Serial.println(current->TaskID);

                    free(temp);
                    Return_code = 1;
                }
            }

            SCH_Print_Tasks(); // Print the task list after deletion
        }

        return Return_code;
    }

    void SCH_Dispatch_Tasks(void)
    {
        if (head != NULL && head->RunMe > 0)
        {
            void (*pTask)() = head->pTask;
            uint32_t Period = head->Period;
            uint32_t TaskID = head->TaskID;

            Serial.print("Dispatching task ID ");
            Serial.println(TaskID);

            // Execute the task
            (*pTask)();

            // Delete the task - it's always at the head
            SCH_Delete_Task(TaskID);

            // If periodic, add it back with the period as the new delay
            if (Period != 0)
            {
                Serial.print("Re-adding periodic task ID ");
                Serial.print(TaskID);
                Serial.print(" with period ");
                Serial.println(Period);
                SCH_Add_Task(pTask, Period, Period);
            }
        }
    }

    // Function to print all tasks in the list
    void SCH_Print_Tasks(void)
    {
        Serial.println("\n--- Task List ---");
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
    }

    // For Arduino - uncomment and adapt for your environment
    // // Init TIMER 10ms
    // static void TIMER_Init()
    // {
    //     cli(); // Disable interrupts
    //     /* Reset Timer/Counter1 */
    //     TCCR1A = 0;
    //     TCCR1B = 0;
    //     TIMSK1 = 0;
    //     /* Setup Timer/Counter1 */
    //     TCCR1B |= (1 << CS11) | (1 << CS10); // prescale = 64
    //     TCNT1 = COUNTER_START;               // 65536 - 10000/(64/16)
    //     TIMSK1 = (1 << TOIE1);               // Overflow interrupt enable
    //     sei();                               // Enable interrupts
    // }
    // ISR(TIMER1_OVF_vect)
    // {
    //     TCNT1 = COUNTER_START;
    //     SCH_Update();
    // }

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
        while (true)
        {
            SCH_Update();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    void initSetUpScheduler()
    {
        xTaskCreate(TaskSchedule, "SchedulerTask", 2048, NULL, 1, &TaskScheduler);
    }

#ifdef __cplusplus
}
#endif