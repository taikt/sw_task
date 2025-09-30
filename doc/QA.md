# Software Quality Attributes Comparison: SW Task vs Tiger Looper

## Performance

### sw_task:

**a> Multi-Thread Architecture for Different Workloads**
   - Main thread for UI/IO events (non-blocking)
   - CPU worker thread for heavy computations
   - Clean separation prevents blocking

**b> Introduce concurrency using promise**
   - Use promise object to handle async operation
   - Support promise chain, handling exception
   - Support backward compability  

### tiger:
- Sequential processing model (single event thread)  
- All workloads on same thread (UI + CPU + Timers)
- No workload isolation, blocking issues
- Hanlding async operation based on message,handler,looper.


## Efficiency (Optimal resource utilization)

### sw_task:

**a> Advanced Timer Management**
   - Dedicated timer thread using Linux epoll + timerfd
   - Single dedicated timer thread vs multiple SIGEV_THREAD
   - O(1) epoll operations vs O(n) signal handling
   - Minimal thread overhead: 1000 timers = 1 thread only
   - Kernel-level timer events with optimal resource usage


### tiger:
- Thread proliferation: each timer creates new thread (SIGEV_THREAD)
- Memory waste: 100 timers = 100 × 8MB stack memory
- Context switch overhead from multiple timer threads


## Userability

### sw_task:

**a> Facade Pattern - hide system complexity**
   [Facade_Pattern_Diagram]
   - SLLooper provides unified interface to EventQueue subsystem
   - Multiple programming interfaces: Message, Function, Promise
   - Single point of control with multiple access methods
   
**b> Multiple Programming paradigms**
   - Message API (backward compatible)
   - Function API (modern C++)
   - Promise API (async chain)

### tiger:
- Monolithic interface design
- No facade abstraction
- Single access pattern only


## Reusability

### sw_task:

**a> Backward Compatible API Enhancement**
   - Preserves Tiger Looper Message API for existing code
   - Adds Function and Promise APIs to same classes
   - Unified processing for both legacy and modern approaches
   - Direct class evolution without breaking existing clients

**b> No backward compatible API**

## diagram type
static: module view
dynamic: sequence diagram, communication diagram, component and connector diagram