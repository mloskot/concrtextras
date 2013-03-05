What is New

  New classes
  - task and task_completion_event

  New samples that demonstrate use of PPL tasks:
  - Proj build: Demonstrates use of continuations to schedule dependent tasks. 
  - GUI Sample: Using continuations in a GUI application. 
  - Timeout: demonstrates adding a timeout to long-running operations.
  - Make: Generalized project build utility with project dependencies defined in DGML, demonstrates capabilities of the when_all function.
  - Sequence Alignment - uses tasks and continuations to solve the dynamic programming local-alignment problem


Directory Structure

  There are two directories:

  1. ConcRTExtras: contains header files that provide containers and algorithms

  2. Samples: contain a number of sample applications that demonstrate the use of parallel pattern library (ppl.h), 
	      the agents library (agents.h), and a number of extras provided in this sample pack.

  ConcRT Extras

   Contains:
   - agents_extras.h
   - barrier.h
   - bounded_queue.h
   - concrt_extras.h
   - concurrent_unordered_map.h
   - concurrent_unordered_set.h
   - connect.h
   - internal_concurrent_hash.h
   - internal_split_ordered_list.h
   - ppl_extras.h
   - semaphore.h
   - ppltasks.h


  Samples

   Contains:
   - Carmichaels
   - Cartoonizer
   - DiningPhilosophers
   - Event
   - ExcelDemo
   - Finonacci
   - FindFirst
   - FindString
   - Parallel_for_flavors
   - ParallelSortDemo
   - ProjectBuild
   - GUI_Tasks
   - Timeout
   - Make
   - SequenceAlignment
   

