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
- `type`: the type of the attribute which must be one of `integer`, `decimal`, `boolean`, `string`, `collection`,
- `name`: the name of the attribute and optionally, an initial assignment 
- `objective`: an optional field indicating whether the attribute value contributes to a global objective which must be either `maximize` or `minimize`, and
- `weight`: an optional decimal indicating a multiplier for the objective function which must be provided if `objective` is set.


### Global attributes

Global attributes can be defined globally for a @ref XML::bpmn::tCollaboration "collaboration element" as shown in the following example.
```xml
<bpmn2:extensionElements>
  <bpmnos:attributes>
    <bpmnos:attribute id="Makespan" name="makespan := 0" type="decimal" objective="minimize" weight="1" />
  </bpmnos:attributes>
</bpmn2:extensionElements>
```

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

@note Modifying data attribute values may lead to race conditions. However, when data attributes are only modified through activities within @ref BPMNS::Model::SequentialAdHocSubProcess "ad-hoc subprocesses with sequential ordering" and the respective performer garantees sequential execution of all activties modifying a data attribute value, race conditions can be prevented. 

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
Message definitions may contain one or more parameters defining a message header, where the `name` field represents a name for the header entry and either the `attribute` field provides an attribute name holding the value of the header entry or the `value` field explicitly specifies the value.
By default, every header contains entries with names `sender` and `recipient`.
Messages can only be deliered to a recipient if the recipient specifies the same message name as the sender and the entry names of the headers are identical and all header values match, i.e. either have the same value or one of both is undefined.
 
Moreover, message definitions may contain one or more `<bpmnos:content>` element defining a message content.
For each such element the following fields can be provided
- `key`: a key allowing to refer to the content,
- `attribute`: the attribute name containing the value to be added to the message content or the name of the attribute for which the value is set to the message content.

The following shows an example of an outgoing message definition for a @ref BPMN::SendTask "send task" sending a message with name `Request` to a recipient that must also have a parameter named `machine` having the value of the `machine` attribute. The message content is populated with the values of the `instance` and `duration`  attributes.
```xml
<bpmn2:extensionElements>
  <bpmnos:messages>
    <bpmnos:message name="Request">
      <bpmnos:parameter name="machine" value="machine" />
      <bpmnos:content key="Order" attribute="instance" />
      <bpmnos:content key="Duration" attribute="duration" />
    </bpmnos:message>
  </bpmnos:messages>
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
@note 
For @ref BPMN::SendTask "send tasks" and @ref BPMN::ReceiveTask "receive tasks"  the `<bpmnos:message>` element must be embedded in a `<bpmnos:message>` container because multiple messages may be defined for multi-instance tasks. 
For other @ref BPMN::MessageCatchEvent "message catch events" and @ref BPMN::MessageThrowEvent "message throw events" the `<bpmnos:message>` element must **not** be embedded in a `<bpmnos:message>` container. 


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

Lookup tables can be made available by adding the following extension elements to a data store reference. 
```xml
<bpmn2:extensionElements>
  <bpmnos:tables>
     <bpmnos:table id="Table_0udt1qg" name="costs" source="costs.csv" />
  </bpmnos:tables>
</bpmn2:extensionElements>
```
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

