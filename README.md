# `riften::Deque` 

A bleeding-edge lock-free, single-producer multi-consumer, Chase-Lev work stealing deque as presented in the paper "Dynamic Circular Work-Stealing Deque" and further improved in the follow up paper: "Correct and Efficient Work-Stealing for Weak Memory Models". 

There exists a few implementations of this data structure: 
- https://github.com/ssbl/concurrent-deque
- https://github.com/taskflow/work-stealing-queue

However, (to my best knowledge) no implementation that can handle 