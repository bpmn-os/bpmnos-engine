@page knapsack_problem Knapsack problem
# Collaboration `Knapsack_problem`
Given a set of items, each having a specifed weight and value, and a knapsack with limited capacity, the *Knapsack problem*  can be described as the problem of finding a selection of items to be included in the knapsack such that
- the total weight of the selected items does not exceed the capacity of the knapsack, and
- the total value of the selected items is maximised.
## Diagram
\htmlonly
<object data="./Knapsack_problem.svg" type="image/svg+xml" style="max-width: 100%;">Knapsack_problem</object>
\endhtmlonly


## Process `ItemProcess`
The *Item process* starts with sending a request messsage including the item identifier, its `weight` and its `value`. Depending on whether the knapsack reject or accepts the item for inclusion, the respective end event is reached.
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" value="0">
			</bpmnos:attribute>
			<bpmnos:attribute id="Value" name="value" type="decimal">
			</bpmnos:attribute>
			<bpmnos:attribute id="Weight" name="weight" type="decimal">
			</bpmnos:attribute>
		</bpmnos:attributes>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## DataObject `DataObject_1xt50y3`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Instance" name="instance" type="string">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```


## Task `SendRequestTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:messages>
		<bpmnos:message name="Request">
			<bpmnos:content attribute="instance" id="Content_2uibfna" key="Item">
			</bpmnos:content>
			<bpmnos:content attribute="value" id="Content_2bfg7g9" key="Value">
			</bpmnos:content>
			<bpmnos:content attribute="weight" id="Content_3klisrm" key="Weight">
			</bpmnos:content>
		</bpmnos:message>
	</bpmnos:messages>
</bpmn2:extensionElements>
```


## Event `CatchAcceptanceMessage`
```xml
<bpmn2:extensionElements>
	<bpmnos:message name="Acceptance">
	</bpmnos:message>
</bpmn2:extensionElements>
```


## Event `CatchRejectionMessage`
```xml
<bpmn2:extensionElements>
	<bpmnos:message name="Rejection">
	</bpmnos:message>
</bpmn2:extensionElements>
```


## Process `KnapsackProcess`
The *Knapsack process* awaits requests for inclusion and spawns an event-subprocess for each item requesting inclusion.

It has a data object containing the `capacity`as well as the `total_weight` and `total_value` of all items included.
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" value="0">
			</bpmnos:attribute>
		</bpmnos:attributes>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## DataObject `DataObject_0vq7lh8`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Capacity" name="capacity" type="decimal">
		</bpmnos:attribute>
		<bpmnos:attribute id="TotalWeight" name="total_weight" type="decimal" value="0">
		</bpmnos:attribute>
		<bpmnos:attribute id="TotalValue" name="total_value" objective="maximize" type="decimal" value="0" weight="1">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```


## DataObject `DataObject_2xt50y3`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Instance" name="instance" type="string">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```


## Task `WaitActivity`
@attention Item requests can not be received after this task is completed and the token is forwarded to the end event.

## Event-SubProcess `EventSubProcess`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Item" name="item" type="string">
			</bpmnos:attribute>
			<bpmnos:attribute id="Weight" name="weight" type="decimal">
			</bpmnos:attribute>
			<bpmnos:attribute id="Value" name="value" type="decimal">
			</bpmnos:attribute>
		</bpmnos:attributes>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Ad-hoc SubProcess `AdHocSubProcess`
The adhoc-subprocess ensures that the *Handle item* activities of all event-subprocesses are executed sequentially.

## SubProcess `HandleItemActivity`
This activity checks whether the item can be included in the knapsack without exceeding its capacity and rejects or accepts the item accordingly.

## SequenceFlow `Flow_0y2js91`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_222eo7i">
			<bpmnos:parameter name="linear" value="total_weight + weight > capacity">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_1r0tr4t`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_1g0l6u8">
			<bpmnos:parameter name="linear" value="total_weight + weight  <= capacity">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## Event `ThrowRejectionMessage`
```xml
<bpmn2:extensionElements>
	<bpmnos:message name="Rejection">
		<bpmnos:parameter attribute="item" name="recipient">
		</bpmnos:parameter>
	</bpmnos:message>
</bpmn2:extensionElements>
```


## Task `IncludeItemActivity`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="total_value" id="Operator_30a3lhs" type="expression">
				<bpmnos:parameter name="linear" value="total_value + value">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="total_weight" id="Operator_33i1d3i" type="expression">
				<bpmnos:parameter name="linear" value="total_weight + weight">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Event `ThrowAcceptanceMessage`
```xml
<bpmn2:extensionElements>
	<bpmnos:message name="Acceptance">
		<bpmnos:parameter attribute="item" name="recipient">
		</bpmnos:parameter>
	</bpmnos:message>
</bpmn2:extensionElements>
```


## Event `CatchRequestMessage`
```xml
<bpmn2:extensionElements>
	<bpmnos:message name="Request">
		<bpmnos:content attribute="item" id="Content_1fan2i2" key="Item">
		</bpmnos:content>
		<bpmnos:content attribute="weight" id="Content_06b5vbt" key="Weight">
		</bpmnos:content>
		<bpmnos:content attribute="value" id="Content_08oldrv" key="Value">
		</bpmnos:content>
	</bpmnos:message>
</bpmn2:extensionElements>
```
