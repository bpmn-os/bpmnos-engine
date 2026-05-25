# Issue #50: Support Simulation While Keeping Current State

## Implementation Strategy for Scenario Alternatives

### Analysis Summary

After code review, the sampling logic is well-contained in `StochasticScenario`, and scenario creation is properly abstracted through the `Scenario` base class. This suggests using a virtual factory method on the base class rather than a copy constructor.

**Key findings:**
- `StochasticDataProvider` stores deferred expressions (just strings) that reference initialization and disclosure expressions
- `StochasticScenario::evaluateDeferredDisclosures()` (lines 50-129 in StochasticScenario.cpp) handles all per-scenario sampling with scenario-specific RNG
- Each (instance, node) pair gets its own seeded RNG created from `scenarioSeed` (line 214)
- Scenario creation follows a factory pattern through `DataProvider::createScenario()`

### Recommended Design: `Scenario::createAlternative(...)`

Use a **virtual method on the `Scenario` base class** for maximum polymorphism and encapsulation:

```cpp
// Scenario.h - abstract base class
virtual std::unique_ptr<Scenario> createAlternative(
  BPMNOS::number currentTime,
  unsigned int newSeed
) = 0;
```

#### **Why This Approach**

✅ **Polymorphic at the Right Level** - Works with ANY scenario specialization without knowing the type  
✅ **Encapsulation** - Scenario knows how to create alternatives of itself  
✅ **Symmetry** - `createScenario()` (provider) creates initial, `createAlternative()` (scenario) creates alternatives  
✅ **Specialization Support** - Each scenario type handles its own alternative creation logic  
✅ **Single Responsibility** - No coupling between provider and alternative creation  

#### **Usage Pattern**

```cpp
// Create initial scenario from provider
auto scenario1 = provider.createScenario(0);

// Create alternative scenarios from existing scenario
auto scenario2 = scenario1->createAlternative(currentTime, newSeed);
auto scenario3 = scenario1->createAlternative(currentTime, newSeed);
```

### Implementation Per Scenario Type

#### **StochasticScenario::createAlternative()**

```cpp
// StochasticScenario.h
std::unique_ptr<Scenario> createAlternative(
  BPMNOS::number currentTime,
  unsigned int newSeed
) override;
```

**Implementation approach:**

1. **Create new scenario instance** with the new seed
2. **Copy non-future state** from current scenario:
   - Instances and their values up to `currentTime`
   - Disclosure times
   - Pending disclosures for past times
3. **Resample future initializations** using `evaluateDeferredDisclosures()` with filter:
   - Only resample deferred expressions with `disclosureTime > currentTime`
   - Clear RNGs to force recreation with new seed
   - Remove pending disclosures for items being resampled
4. **Return as `unique_ptr<Scenario>`**

**Key implementation details:**
- Modify `evaluateDeferredDisclosures()` to support filtering:
  ```cpp
  void evaluateDeferredDisclosures(
    BPMNOS::number resampleAfterTime = std::numeric_limits<BPMNOS::number>::lowest()
  );
  ```
  - Default parameter evaluates ALL (initial sampling)
  - With `currentTime` parameter, only evaluates future initializations
- Do NOT clear `deferredDisclosures` after initial sampling to allow resampling
- Reuse existing RNG seeding logic in `getRng()` - new seed automatically creates new RNG values

#### **StaticScenario::createAlternative()**

```cpp
std::unique_ptr<Scenario> StaticScenario::createAlternative(
  BPMNOS::number currentTime,
  unsigned int newSeed
) override {
  // For static scenarios, seed doesn't matter - all data is known from start
  // Just return a copy
  return std::make_unique<StaticScenario>(*this);
}
```

Requires `StaticScenario` to be copyable (part of item 1 in the checklist: "Make `SystemState` copyable").

#### **ExpectedValueScenario::createAlternative()**

```cpp
std::unique_ptr<Scenario> ExpectedValueScenario::createAlternative(
  BPMNOS::number currentTime,
  unsigned int newSeed
) override {
  // For expected value scenarios (deterministic), seed doesn't matter
  return std::make_unique<ExpectedValueScenario>(*this);
}
```

### Implementation Considerations

**State Management:**
- Deferred expressions must NOT be cleared after initial sampling (change line 125 in StochasticScenario.cpp)
- Only clear/modify pending disclosures for initializations being resampled
- RNGs are recreated on-demand with new seed via `getRng()`

**Filtering Logic for `resampleAfterTime`:**
- Evaluate disclosure time first
- Skip if `disclosureTime <= resampleAfterTime`
- Remove from `pendingDisclosures` for resampled items
- Clear corresponding RNGs to force recreation

**Distribution Change Note:**
As mentioned in the issue, when initializations are forced to the next point in time (due to limited resampling tries), the distribution changes. This is acceptable but should be documented.

### Next Steps

1. Add `createAlternative()` pure virtual method to `Scenario` base class
2. Implement in `StochasticScenario` with deferred expression resampling
3. Implement in `StaticScenario` (simple copy)
4. Implement in `ExpectedValueScenario` (simple copy)
5. Handle `SystemState` copyability requirement (item 1 in checklist)
6. Add comprehensive tests for `createAlternative()` with various time points
7. Test with multiple scenario specializations to ensure polymorphism works correctly

