# `riften::Deque` 

A bleeding-edge lock-free, single-producer multi-consumer, Chase-Lev work stealing deque as presented in the paper "Dynamic Circular Work-Stealing Deque" and further improved in the follow up paper: "Correct and Efficient Work-Stealing for Weak Memory Models". 

This implementation is based on:
- https://github.com/taskflow/work-stealing-queue

But places no constraint on the types which can be placed in the deque.