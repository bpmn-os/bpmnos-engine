# BPMN extension
@page extension BPMN extension

Data required for optimisation and simulation is provided through @ref BPMN::ExtensionElements "BPMN extension elements", i.e. 
- @ref BPMNOS::Model::ExtensionElements "extension elements for nodes (except timer events)", 
- @ref BPMNOS::Model::ExtensionElements "extension elements for timer events", and
- @ref BPMNOS::Model::Gatekeeper "extension elements for sequence flows".

The XML-schema definition for these extensions is provided in @ref BPMNOS.xsd. 


## Attributes

Information is provided via @ref BPMNOS::Model::Attribute "attributes".
Within an `<bpmnos:attributes>` container any number of `<bpmnos:attribute>` elements can be provided to define attributes. 
For each attribute the following fields can be provided
- `id`: a unique identifier, 
- `type`: the type of the attribute, a scalar type or the shape of an object as described below,
- `name`: the name of the attribute and optionally, an initial assignment 
- `objective`: an optional field indicating whether the attribute value contributes to a global objective which must be either `maximize` or `minimize`, and
- `weight`: an optional decimal indicating a multiplier for the objective function which must be provided if `objective` is set.

The scalar types are `integer`, `decimal`, `boolean`, `string` and `collection`. Only an attribute of type `boolean`, `integer` or `decimal` may contribute to the objective.

### Objects

An attribute whose type is not a single scalar type is an object, i.e. an array or a structured value. Its type states its shape: a base, which is a scalar type or a list of fields `{ name: type, ... }` each with a type of its own, followed by any number of dimensions, each written `[n]` if its size is fixed and `[]` if it is open. The dimensions are read from left to right in the order of the indices, and each field has dimensions of its own, so that an access has the shape of the type:

| Type | Access |
|------|--------|
| `integer[10]` | `location[i]` with `i <= 10` |
| `boolean[3][4]` | `grid[i][j]` with `i <= 3` and `j <= 4` |
| `{ cost: decimal, flags: boolean[] }[]` | `facilities[i].cost`, `facilities[i].flags[j]` |
| `{ x: decimal, y: decimal }` | `depot.x` |

```xml
<bpmnos:attribute id="Location" name="location" type="integer[10]" />
<bpmnos:attribute id="Facilities" name="facilities" type="{ cost: decimal, flags: boolean[] }[]" />
```

Objects may be declared as global, data and status attributes. They are numbered separately from the scalar attributes, so that the index of a scalar attribute never depends on the objects declared: the global objects come first, and the objects a scope declares follow those of its enclosing scopes. An object cannot be the attribute of a choice, cannot carry the objective, and is neither the instance nor the timestamp.

An object may be initialised by a literal, `name="depot := { x := 0, y := 0 }"`, which every instance shares; the instance data may then give it no value. Otherwise an object takes the value the instance data gives it or undefined values, its sizes coming from its type, from the size declarations of the instance data, and from its value, as described for the @ref BPMNOS::Execution::StaticDataProvider "static data provider". An object is created with the status or data it belongs to: a global object with the global state machine, a data object with the state machine of its scope, and a status object with the status of the token entering its scope, from which it is removed when the scope is left.


### Global attributes

Global attributes can be defined for a @ref XML::bpmn::tDataStore "data store element" as shown in the following example.
```xml
<bpmn2:dataStore id="DataStore_1">
  <bpmn2:extensionElements>
    <bpmnos:attributes>
      <bpmnos:attribute id="Makespan" name="makespan := 0" type="decimal" objective="minimize" weight="1" />
    </bpmnos:attributes>
  </bpmn2:extensionElements>
</bpmn2:dataStore>
```

The objective value of a run is held by the @ref BPMNOS::Execution::SystemState "system state" and is not an attribute. It is the sum of the values of all weighted attributes multiplied by their weights: a global or data attribute contributes from its creation on and by every change of its value, and a status attribute contributes with its final value when its scope ends. Every change of the objective is announced by the @ref BPMNOS::Execution::Objective "objective" notification.

Global attributes are visible to every process of a model, whether or not a process refers to the data store declaring them. They are data attributes held by the global state machine of a run, which holds the token of every process instance, and every scope inherits them as the first part of its data: the global attributes have the indices below the number of global attributes, the instance attribute every process declares first has that number as its index, and the further data attributes of a scope follow the data of its enclosing scopes. A write to a global attribute concerns every instance, a write to any other data attribute only the instance it belongs to.

@note Modifying global attribute values may lead to race conditions.

### Data attributes

Data attributes can be defined for a @ref XML::bpmn::tDataObject "data object element" as shown in the following example.

```xml
<bpmn2:extensionElements>
  <bpmnos:attributes>
    <bpmnos:attribute id="Instance" name="instance" type="string" />
  </bpmnos:attributes>
</bpmn2:extensionElements>
```

Data attributes are similar to global attributes, however, they only exist within the @ref BPMN::Scope "scope" cotaining the @ref BPMN::DataObject "data object".

@note Modifying data attribute values may lead to race conditions. However, when data attributes are only modified through activities within @ref BPMNOS::Model::SequentialAdHocSubProcess "ad-hoc subprocesses with sequential ordering" and the respective performer garantees sequential execution of all activties modifying a data attribute value, race conditions can be prevented. 

### Status attributes

Status attributes can be defined for a @ref XML::bpmn::tProcess "process element" and an @ref XML::bpmn::tActivity "activity element" as shown in the following example.

```xml
<bpmn2:extensionElements>
  <bpmnos:status>
    <bpmnos:attributes>
      <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
    </bpmnos:attributes>
  </bpmnos:status>
</bpmn2:extensionElements>
```

Status attributes are attached to @ref BPMNOS::Execution::Token "tokens" moving through a process model. Their lifetime is restricted to the node that they are defined for and they are dynamically added and removed to the token status.

## Restrictions

@ref BPMNOS::Model::Restriction "Restrictions" allow to constrain the domain of attributes and to determine the sequence flows out of diverging gateways.
Within a `<bpmnos:restrictions>` container any number of `<bpmnos:restriction>` elements can be provided to specify restrictions. 
For each restriction the following fields can be provided
- `id`: a unique identifier, 
- `scope`: the scope indicating when the restriction has to be satisfied which must be one of  `entry`, `exit`, `full`.
- `expression`: an expression representing the requirements.
 

### Node restrictions 
Restrictions can be added to a @ref XML::bpmn::tProcess "process element" and an @ref XML::bpmn::tActivity "activity element" as shown in the following example.

```xml
<bpmn2:extensionElements>
  <bpmnos:status>
    <bpmnos:restrictions>
      <bpmnos:restriction id="Restriction_3br81vk" expression="total_size &#60;= capacity" />
    </bpmnos:restrictions>
  </bpmnos:status>
</bpmn2:extensionElements>
```

### Gatekeeper restrictions 
Gatekeeper restrictions can be added to a @ref XML::bpmn::tSequenceFlow "sequence flow element" as shown in the following example.

```xml
<bpmn2:extensionElements>
  <bpmnos:restrictions>
    <bpmnos:restriction id="Restriction_222eo7i" expression="total_weight + weight &#62; capacity" />
    </bpmnos:restriction>
  </bpmnos:restrictions>
</bpmn2:extensionElements>
```

## Operators
@ref BPMNOS::Model::Operator "operators" can be used to modify attribute values.
Within an `<bpmnos:operators>` container any number of `<bpmnos:operator>` elements can be provided to specify operators. 
For each operator the following fields can be provided
- `id`: a unique identifier, 
- `expression`: an expression assigning a value to an attribute.

Operators may be provided for @ref BPMN::Process "processes", @ref BPMN::SubProcess "subprocesses", @ref BPMN::EventSubProcess "event-subprocesses", and @ref BPMN::Task "tasks".
The operators of a task are applied when the task is performed, after its entry restrictions have been checked.
The operators of an element with a scope are applied to the token at the start event of that scope upon its completion, the scope being instantiated there, and the entry restrictions of the scope are checked thereafter, so that they constrain the status the operators produce.
A start event has no operators of its own.
A scope is instantiated as often as it is entered, so the operators of a looped or of a multi-instance subprocess are applied once per iteration and once per instance respectively.
An @ref BPMN::AdHocSubProcess "ad-hoc subprocess" has no start event and must therefore not declare operators.

@attention Operators for elements with a scope must not modify the `timestamp` attribute, a scope being entered instantaneously and its duration being the duration of what happens within it. The same holds for a @ref BPMN::SendTask "send task", a @ref BPMN::ReceiveTask "receive task" and a @ref BPMNOS::Model::DecisionTask "decision task", each of which completes upon an event of its own. Only a regular task may advance the timestamp, which is how its duration is modelled.

The following shows an example of an operator increasing an attribute value.
  ```xml
  <bpmn2:extensionElements>
    <bpmnos:status>
      <bpmnos:operators>
        <bpmnos:operator id="Operator_30a3lhs" expression="total_value += value" />
      </bpmnos:operators>
    </bpmnos:status>
  </bpmn2:extensionElements>
  ```
  
The following shows an example using a lookup table that must be specified using a data store.
  ```xml
  <bpmn2:extensionElements>
    <bpmnos:status>
      <bpmnos:operators>
        <bpmnos:operator id="Operator_2ldkdhd" expresion="cost := cost(client,instance)" />
      </bpmnos:operators>
    </bpmnos:status>
  </bpmn2:extensionElements>
  ```

## Choices
@ref BPMNOS::Model::DecisionTask "Decision tasks" are represented by @ref XML::bpmn::tTask "task elements" with an additional field `bpmnos:type="Decision"`. 
For these decision tasks, @ref BPMNOS::Model::Choice "choice" on the value fo one or more attributes can be made.
Within a `<bpmnos:decisions>` container any number of `<bpmnos:decision>` elements can be provided to define the respective attributes as shown in the following example. 

```xml
<bpmn2:extensionElements>
  <bpmnos:status>
     <bpmnos:decisions>
       <bpmnos:decision id="Decision_0udt1qg" condition="wait_type in [&#34;wait&#34;, &#34;break&#34;, &#34;rest&#34;]" />
     </bpmnos:decisions>
  </bpmnos:status>
</bpmn2:extensionElements>
```

Each condition constrains the values that may be chosen for the specified attribute.

## Messages
@ref BPMNOS::Model::MessageDefinition "Messages" can be used to exchange information by delivering a @ref BPMNOS::Model::Content "content" from one process to another. 

For @ref BPMN::MessageThrowEvent "message throw events" and @ref BPMN::MessageCatchEvent "message catch events" a `<bpmnos:message>` element must be provided with field `name` representing a name of the message.
Message definitions may contain one or more parameters defining a message header, where the `name` field represents a name for the header entry and the `value` field states the value held under it.
The value states the name of a declared attribute, in which case the entry holds the value of that attribute and is of the type the attribute is declared with, or a quoted string, in which case the entry holds that string.
Nothing else may be stated, so that the type of every entry is known when the model is read.
A parameter that states no value at all is permitted and means that the entry holds no value, which every value held under that name matches.
By default, every header contains entries with names `sender` and `recipient`, which hold instance identifiers, and an entry with name `name`, which holds the message name.
Messages can only be delivered to a recipient if the recipient specifies the same message name as the sender, the entry names of the headers are identical, the entries of the same name are of the same type wherever both sender and recipient state one, and all header values match, i.e. either have the same value or one of both is undefined.
 
Moreover, message definitions may contain one or more `<bpmnos:content>` element defining a message content.
For each such element the following fields can be provided
- `key`: a key allowing to refer to the content,
- `attribute`: the attribute name containing the value to be added to the message content or the name of the attribute for which the value is set to the message content.

The following shows an example of an outgoing message definition for a @ref BPMN::SendTask "send task" sending a message with name `Request` to a recipient that must also have a parameter named `machine` having the value of the `machine` attribute. The message content is populated with the values of the `instance` and `duration`  attributes.
```xml
<bpmn2:extensionElements>
  <bpmnos:message name="Request">
    <bpmnos:parameter name="machine" value="machine" />
    <bpmnos:content key="Order" attribute="instance" />
    <bpmnos:content key="Duration" attribute="duration" />
  </bpmnos:message>
</bpmn2:extensionElements>
```

The following shows an example of an incoming message definition for a @ref BPMN::MessageCatchEvent "message catch event" receiving a message with name `Request` from a sender that must also have a parameter named `machine` having the value of the `machine` attribute. The recipient attributes `order` and `duration` are set to the values of the respective message content.
```xml
<bpmn2:extensionElements>
  <bpmnos:message name="Request">
    <bpmnos:parameter name="machine" value="machine" />
    <bpmnos:content key="Order" attribute="order" />
    <bpmnos:content key="Duration" attribute="duration" />
  </bpmnos:message>
</bpmn2:extensionElements>
```

A message may also instantiate a process, which it does where the process has a @ref BPMN::MessageStartEvent "message start event".
Such a message names no recipient, the instance not existing before the message arrives, and is matched by its name alone.
No element other than that start event may therefore catch a message of that name, and neither the start event nor any element throwing the message may state header parameters or a recipient.
Several elements may throw it, each instantiating a process of its own, and a multi-instance @ref BPMN::SendTask "send task" instantiates one process per instance.
A process cannot send itself a message, a message flow connecting two participants, so the message must be thrown outside the process it instantiates.
The instance receives a generated identifier and is created with the content of the message in its status.

## Signals
@ref BPMNOS::Model::SignalDefinition "Signals" can be used to broadcast a @ref BPMNOS::Model::Content "content" to whoever is listening for them.

For @ref BPMN::SignalThrowEvent "signal throw events" and @ref BPMN::SignalCatchEvent "signal catch events" a `<bpmnos:signal>` element must be provided with field `name` representing a name of the signal, and it may contain one or more `<bpmnos:content>` elements defining the content the signal carries, each with the fields `key` and `attribute` that a message content has.

A signal has no header and no parameters, and this is what distinguishes it from a message. A message is delivered to one recipient, which is chosen among those a message flow and a matching header admit; a signal is broadcast to every element waiting for a signal of that name at the moment it is thrown, and to none if nothing is waiting, in which case it is simply lost. It states no sender and no recipient, so a model that wants the thrower known declares it as part of the content, as the following example does.

```xml
<bpmn2:extensionElements>
  <bpmnos:signal name="Cancellation">
    <bpmnos:content key="Origin" attribute="instance" />
    <bpmnos:content key="Reason" attribute="reason" />
  </bpmnos:signal>
</bpmn2:extensionElements>
```

The content is read from the attributes of the throwing element when the signal is thrown, and it is fixed at that moment, so that a recipient modifying a global attribute cannot change what another recipient receives. Each recipient declares under the same key the attribute the value is written to, and those attributes need not be named alike, which is what the key is for. A key a recipient declares but the signal does not carry leaves that attribute undefined.

A signal may also instantiate a process, which it does where the process has a @ref BPMN::SignalStartEvent "signal start event". Such a process is instantiated whenever a signal of that name is thrown, including by itself, the specification giving no ground to forbid a process instantiating itself where a signal rather than a message carries the trigger. An unguarded re-throw does not terminate.

## Timer
The trigger for a @ref BPMN::TimerCatchEvent "timer event" can be specified by providing a parameter with name `trigger` and value being an expression as shown in the following example.

```xml
<bpmn2:extensionElements>
  <bpmnos:timer>
    <bpmnos:parameter name="trigger" value="timestamp + 5" />
  </bpmnos:timer>
</bpmn2:extensionElements>
```

## Lookup tables

Lookup tables can be made available by adding the following extension elements to a @ref XML::bpmn::tDataStore "data store element". 
```xml
<bpmn2:dataStore id="DataStore_2">
  <bpmn2:extensionElements>
    <bpmnos:tables>
      <bpmnos:table id="Table_0udt1qg" name="costs" source="costs.csv" />
    </bpmnos:tables>
  </bpmn2:extensionElements>
</bpmn2:dataStore>
```
The name of a lookup table can be used in every expression of a model, whether or not a process refers to the data store declaring it.
The parameter `name` specifies the name of the lookup table to be used in expressions. 
The parameter `source` specifies the filen name of the lookup table. 

  @note Currently, the only supported source are csv files.
  @par
  @note The folders to search for lookup table files can be provided by adding them to the constructor of the model or data provider.


## Loop parameters

For loop and multi-instance activities, additional parameters can be specified: 
- The @ref BPMNOS::Model::ExtensionElements::loopCardinality "cardinality" parameter specifies the number of instances to be generated (only for multi-instance activities).
- The @ref BPMNOS::Model::ExtensionElements::loopIndex "index" parameter specified the name of an attribute in which the index of the instance is stored (only for multi-instance activities).
- The @ref BPMNOS::Model::ExtensionElements::loopCondition "condition" parameter specifies an attribute whos boolean value must be true in order to continue with another loop (only for loop activities).
- The @ref BPMNOS::Model::ExtensionElements::loopMaximum "maximum" parameter specifies the maximum number of loops that may be conducted (only for loop activities).

