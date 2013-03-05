The following projects are included in this solution:

ConcRT Extras:
This folder will include extensions to PPL, agents and the Concurrency Runtime which provide additional useful functionality.  This release includes the following files: ppl_extras.h which includes additional parallel algorithms, ppl_agents.h which includes additional agents message blocks and concrt.h which provides useful wrapper classes and utilities.  Also included are concurrent unordered map/multimap/set/multiset, an improved cooperative barrier and the countdown_event. 

Excel Demo:
A demo of an excel plugin that performs a parallel monte carlo computation

N-bodies:
A Direct2D based implementation of n-bodies (requires Windows 7 or Direct2D)

DiningPhilosophers:
A message passing based implementation of the Dining Philosophers Problem, see http://msdn.microsoft.com/en-us/magazine/dd882512.aspx for discussion.

Event:
A short sample which demonstrates using Concurrency::Event and how cooperative blocking differs from standard blocking. This will also highlight how UMS Threads on Win7 x64 enable cooperative blocking for all kernel blocking.

Fibonacci:
A simple recursive task example that shows how to use structured_task_group for parallelism.  This example also demonstrates using the Concurrent Suballocator to improve the performance where there are frequent small allocations and reallocations.

Find First:
This example shows how to use the choice and join classes from the Agents Library to select the first PPL task to complete a search algorithm.  See the documentation for a full description: http://msdn.microsoft.com/en-us/library/dd728075(VS.100).aspx.

Find String:
This is a sample Agents based application that searches a directory structure recursively for a text pattern, similar to findstr but parallel.

Counting Carmichaels:
This is a sample application that shows benefits of nesting parallel loops.

ParallelSortDemo:
A demo contrasting the various implementations of parallel sort included in the sample pack

Proj build:
Demonstrates use of continuations to schedule dependent tasks. 

GUI Sample:
Using continuations in a GUI application. Shows how to write non-blocking UI with regular continuations, continuations on UI thread and the “pumping wait” construct.

Timeout:
This samples demonstrates adding a timeout to long-running operations. The timeout can be added to an arbitrary function, or a task.

Make:
Generalized project build utility with project dependencies defined in DGML. Demonstrates capabilities of the when_all function.

Sequence Alignment:
This local-alignment sample uses the Smith-Waterman approach to find alignment score between 2 given sequences. This problem falls under the dynamic programming paradigm and uses tasks and continuations to schedule and execute chunks of work
