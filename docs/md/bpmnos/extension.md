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
- `name`: a name allowing to refer to the attribute in operators, restrictions, ...,
- `type`: the type of the attribute which must be one of `integer`, `decimal`, `boolean`, `string`, `collection`,
- `value`: an optional field providing a default value to be used upon instantiation,
- `objective`: an optional field indicating whether the attribute value contributes to a global objective which must be either `maximize` or `minimize`, and
- `weight`: an optional decimal indicating a multiplier for the objective function which must be provided if `objective` is set.

Attributes of multi-instance activities may contain a @ref BPMNOS::Model::Attribute::collection "collection parameter" specifying whether the attribute value is initialized via an atttibute or a default value of type `collection`.



### Global attributes

Global attributes can be defined globally for a @ref XML::bpmn::tCollaboration "collaboration element" as shown in the following example.
```xml
<bpmn2:extensionElements>
  <bpmnos:attributes>
    <bpmnos:attribute id="Makespan" name="makespan" type="decimal" value="0" objective="minimize" weight="1" />
  </bpmnos:attributes>
</bpmn2:extensionElements>
```

@note Modifying gobal attribute values may lead to race conditions.

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
      <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" value="0" />
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

Moreover, the restriction must contain a  `<bpmnos:parameter>` element specifying the expression to be used when validating whether the restriction is satisfied.
The following fields can be provided for the parameter 
- `name`: the name of the expression type which must be one of `enumeration`, `nullcondition`, `string`, `generic`, `linear`,
- `value`: a string containing the expression text.

The requirements on the expression text for the different types of expressions are the following.

- **Enumeration**
  @par
  An enumeration restriction can be specified by providing a parameter with `name="enumeration"` and `value` being an attribute name followed by the keyword `in` and a collection of allowed values within square brackets and separated by comma. String values must be quoted. An example is shown in the following.
  
  ```xml
  <bpmnos:parameter name="enumeration" value="offduty_type in [&#34;break&#34;,&#34;rest&#34;]" />
  ```
  @note Quotes must be replaced by the the html escape code `&#34;`.
  
- **Null condition**
  @par
  A null condition can be specified by providing a parameter with `name="nullcondition"` and `value` being an attribute name followed by either `==` or `!=` and the  @ref BPMNOS::Keyword "keyword" `undefined`. An example is shown in the following.
  
  ```xml
  <bpmnos:parameter name="nullcondition" value="offduty_type != undefined" />
  ```
  
- **String expression**
  @par
  A string expression can be specified by providing a parameter with `name="sting"` and `value` being an attribute name followed by either `==` or `!=` and the either an attribute name or a quoted string. An example is shown in the following.
  
  ```xml
  <bpmnos:parameter name="string" value="origin != destination" />
  ```
  
- **Linear expression**
  @par
  A linear expression can be specified by providing  parameter with `name="linear"` and a `value` being a left hand side and a right hand side part separated by `<`, `<=`, `>`, `>=`, `==`, or `!=`. The left hand side and right hand side part must be composed of terms sepparated by `+` or `-`  and must only contain numbers, attribute names, or attribute names multiplied or divided with numbers.
  
  ```xml
  <bpmnos:parameter name="linear" value="total_value + value" />
  ```
  @note The symbols `<` and `>` must be replaced by the the html escape codes `&#60;` and `&#62;`.

- **Generic expression**
  @par
  A generic expression can be specified by providing a parameter with `name="generic"` and `value` being an expression supported by the [C++ Mathematical Expression Toolkit Library (ExprTk)](https://www.partow.net/programming/exprtk/). An example is shown in the following.
  
  ```xml
  <bpmnos:parameter name="generic" value="max( earliest_visit - timestamp, max_rest_duration )" />
  ```
  
  

### Node restrictions 
Restrictions can be added to a @ref XML::bpmn::tProcess "process element" and an @ref XML::bpmn::tActivity "activity element" as shown in the following example.

```xml
<bpmn2:extensionElements>
  <bpmnos:status>
    <bpmnos:restrictions>
      <bpmnos:restriction id="Restriction_3br81vk">
        <bpmnos:parameter name="linear" value="total_size &#60;= capacity" />
      </bpmnos:restriction>
    </bpmnos:restrictions>
  </bpmnos:status>
</bpmn2:extensionElements>
```

### Gatekeeper restrictions 
Gatekeeper restrictions can be added to a @ref XML::bpmn::tSequenceFlow "sequence flow element" as shown in the following example.

```xml
<bpmn2:extensionElements>
  <bpmnos:restrictions>
    <bpmnos:restriction id="Restriction_222eo7i">
      <bpmnos:parameter name="linear" value="total_weight + weight &#62; capacity" />
    </bpmnos:restriction>
  </bpmnos:restrictions>
</bpmn2:extensionElements>
```

## Operators
@ref BPMNOS::Model::Operator "operators" can be used to modify attribute values.
Within an `<bpmnos:operators>` container any number of `<bpmnos:operator>` elements can be provided to specify operators. 
For each operator the following fields can be provided
- `id`: a unique identifier, 
- `attribute`: the name of the attribute to be modified, 
- `type`: the type of the expression which must be one of `assign`, `unassign`, `lookup`, `expression`,

Moreover, the restriction may contain one or more  `<bpmnos:parameter>` element specifying the parameters required to define the operator.
The following fields can be provided for the parameter 
- `name`: the name of the parameter,
- `attribute`: the name of the attribute holding the parameter value.
- `value`: the parameter value.
  
The requirements on the parameters for the different types of operators are the following.

- **Assign**
  @par
  The assignment operator requires one parameter with `name="assign"` and either the field `attribute` being set to an attribute name or the field `value` being set to the desired value. The following shows an example.
  ```xml
  <bpmn2:extensionElements>
    <bpmnos:status>
      <bpmnos:operators>
        <bpmnos:operator id="OperatorMachine" attribute="machine" type="assign">
          <bpmnos:parameter name="assign" attribute="instance" />
        </bpmnos:operator>
      </bpmnos:operators>
    </bpmnos:status>
  </bpmn2:extensionElements>
  ```

- **Unassign**
  @par
  The unassign operator sets the value of the attribute to undefined and requires no parameter. The following shows an example.
  ```xml
  <bpmn2:extensionElements>
    <bpmnos:status>
      <bpmnos:operators>
        <bpmnos:operator id="OperatorMachine" attribute="machine" type="unassign" />
      </bpmnos:operators>
    </bpmnos:status>
  </bpmn2:extensionElements>
  ```
  
- **Lookup**
  @par
  The lookup operator allows to lookup a value from a data source. It requires several parameters depending on the type of the source.
  For file sources the following parameters are required:
  - a parameter with `name="source"` and `value="file"`,
  - a parameter with `name="filename"` and `value` being a filename,
  - a parameter with `name="key"` and `value` being the column name in which the value to be looked up is found, and
  - one or multiple parameters with `name` being a column name containing the value specified in the `value` field.
  
  
  The following shows an example.
  ```xml
  <bpmn2:extensionElements>
    <bpmnos:status>
      <bpmnos:operators>
        <bpmnos:operator id="Operator_2ldkdhd" attribute="cost" type="lookup">
          <bpmnos:parameter name="source" value="file" />
          <bpmnos:parameter name="filename" value="costs.csv" />
          <bpmnos:parameter name="key" value="Costs" />
          <bpmnos:parameter name="ClientId" attribute="client" />
          <bpmnos:parameter name="ServerId" attribute="instance" />
        </bpmnos:operator>
      </bpmnos:operators>
    </bpmnos:status>
  </bpmn2:extensionElements>
  ```
  @note Currently, the only supported source are csv files.
  @par
  @note The folders to search for lookup table files can be provided by adding them to @ref BPMNOS::Model::LookupTable::folders as shown in the following example. 
  ```cpp
  BPMNOS::Model::LookupTable::folders = { std::string(std::filesystem::current_path()) + "/examples/assignment_problem" };
  ```

- **Expression**
  @par
  The expression operator requires one parameter with `name` being the name of the expression type and the field `value` being set to expression text. For boolean expressions a `true` value results in `true` for boolean attributes, `"true"` for string attributes and `1` for  integer or decimal attributes. A `false` value results in `false` for boolean attributes, `"false"` for string attributes and `0` for  integer or decimal attributes.   

  The following shows an example.
  ```xml
  <bpmn2:extensionElements>
    <bpmnos:status>
      <bpmnos:operators>
        <bpmnos:operator id="Operator_30a3lhs" attribute="total_value" type="expression">
          <bpmnos:parameter name="linear" value="total_value + value" />
        </bpmnos:operator>
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
    <bpmnos:restrictions>
      <bpmnos:restriction id="Restriction_0uj7fj8" scope="exit">
        <bpmnos:parameter name="enumeration" value="wait_type in [&#34;wait&#34;, &#34;break&#34;, &#34;rest&#34;]" />
      </bpmnos:restriction>
    </bpmnos:restrictions>
     <bpmnos:decisions>
       <bpmnos:decision id="Decision_0udt1qg" attribute="wait_type" />
     </bpmnos:decisions>
  </bpmnos:status>
</bpmn2:extensionElements>
```

@note Restrictions can constrain the attribute values that may be chosen.

## Messages
@ref BPMNOS::Model::MessageDefinition "Messages" can be used to exchange information by delivering a @ref BPMNOS::Model::Content "content" from one process to another. 

For @ref BPMN::MessageThrowEvent "message throw events" and @ref BPMN::MessageCatchEvent "message catch events" a `<bpmnos:message>` element must be provided with field `name` representing a name of the message.
Message definitions may contain one or more parameters defining a message header, where the `name` field represents a name for the header entry and either the `attribute` field provides an attribute name holding the value of the header entry or the `value` field explicitly specifies the value.
By default, every header contains entries with names `sender` and `recipient`.
Messages can only be deliered to a recipient if the recipient specifies the same message name as the sender and the entry names of the headers are identical and all header values match, i.e. either have the same value or one of both is undefined.
 
Moreover, message definitions may contain one or more `<bpmnos:content>` element defining a message content.
For each such element the following fields can be provided
- `id`: a unique identifier, 
- `key`: a key allowing to refer to the content,
- `attribute`: the attribute name containing the value to be added to the message content or the name of the attribute for which the value is set to the message content,
- `value`: an optional field providing a default value to be used.

@note The `value` field may only be specified for @ref BPMN::MessageThrowEvent "message throw events".

The following shows an example of an outgoing message definition for a @ref BPMN::SendTask "send task" sending a message with name `Request` to a recipient that must also have a parameter named `machine` having the value of the `machine` attribute. The message content is populated with the values of the `instance` and `duration`  attributes.
```xml
<bpmn2:extensionElements>
  <bpmnos:messages>
    <bpmnos:message name="Request">
      <bpmnos:parameter name="machine" attribute="machine" />
      <bpmnos:content id="Content_1485jek" key="Order" attribute="instance" />
      <bpmnos:content id="Content_2d4hb1c" key="Duration" attribute="duration" />
    </bpmnos:message>
  </bpmnos:messages>
</bpmn2:extensionElements>
```

The following shows an example of an incoming message definition for a @ref BPMN::MessageCatchEvent "message catch event" receiving a message with name `Request` from a sender that must also have a parameter named `machine` having the value of the `machine` attribute. The recipient attributes `order` and `duration` are set to the values of the respective message content.
```xml
<bpmn2:extensionElements>
  <bpmnos:message name="Request">
    <bpmnos:parameter name="machine" attribute="machine" />
    <bpmnos:content id="Content_331q4tt" key="Order" attribute="order" />
    <bpmnos:content id="Content_3gvltfq" key="Duration" attribute="duration" />
  </bpmnos:message>
</bpmn2:extensionElements>
```
@note 
For @ref BPMN::SendTask "send tasks" and @ref BPMN::ReceiveTask "receive tasks"  the `<bpmnos:message>` element must be embedded in a `<bpmnos:message>` container because multiple messages may be defined for multi-instance tasks. 
For other @ref BPMN::MessageCatchEvent "message catch events" and @ref BPMN::MessageThrowEvent "message throw events" the `<bpmnos:message>` element must **not** be embedded in a `<bpmnos:message>` container. 


## Timer
The trigger for a @ref BPMN::TimerCatchEvent "timer event" can be specified by providing a parameter with name `trigger` within a  `<bpmnos:timer>` element as shown in the following example.

```xml
<bpmn2:extensionElements>
  <bpmnos:timer>
    <bpmnos:parameter name="trigger" attribute="trigger" value="5" />
  </bpmnos:timer>
</bpmn2:extensionElements>
```

## Loop parameters

For multi-instance activities, the number of instances can be specified by @ref BPMNOS::Model::ExtensionElements::loopCardinality "loop cardinality parameter". An attribute containing the index of the instance can be specified by @ref BPMNOS::Model::ExtensionElements::loopIndex "loop index parameter".

