@page bin_packing_problem Bin packing problem
# Collaboration `Bin_packing_problem`
Given a set of items, each having a specifed size, and a set of bins each having a specified capacity, the *Bin packing problem* can be described as the problem of finding the minimal number of bins required to include all items such that the total size of all items associated to a bin does not exceed the capacity of the bin.
## Diagram
\htmlonly
<object data="./Bin_packing_problem.svg" type="image/svg+xml" style="max-width: 100%;">Bin_packing_problem</object>
\endhtmlonly


## Process `ItemProcess`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" value="0">
			</bpmnos:attribute>
			<bpmnos:attribute id="Size" name="size" type="decimal">
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


## Task `Activity_1qepu6x`
```xml
<bpmn2:extensionElements>
	<bpmnos:messages>
		<bpmnos:message name="Request">
			<bpmnos:content attribute="instance" id="SendItemContent" key="Item">
			</bpmnos:content>
			<bpmnos:content attribute="size" id="SendSizeContent" key="Size">
			</bpmnos:content>
		</bpmnos:message>
	</bpmnos:messages>
</bpmn2:extensionElements>
```


## Process `BinProcess`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" value="0">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_3br81vk">
				<bpmnos:parameter name="linear" value="total_size <= capacity">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
	</bpmnos:status>
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


## DataObject `DataObject_1pee096`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Capacity" name="capacity" type="decimal">
		</bpmnos:attribute>
		<bpmnos:attribute id="TotalSize" name="total_size" type="decimal" value="0">
		</bpmnos:attribute>
		<bpmnos:attribute id="Used" name="used" objective="minimize" type="boolean" value="false" weight="1">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```


## Event-SubProcess `EventSubProcess`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Item" name="item" type="string">
			</bpmnos:attribute>
			<bpmnos:attribute id="Size" name="size" type="decimal">
			</bpmnos:attribute>
		</bpmnos:attributes>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `Activity_0m9f4ep`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="used" id="OperatorUsed" type="assign">
				<bpmnos:parameter name="assign" value="true">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="total_size" id="OperatorTotalSize" type="expression">
				<bpmnos:parameter name="linear" value="total_size + size">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Event `CatchRequestMessage`
```xml
<bpmn2:extensionElements>
	<bpmnos:message name="Request">
		<bpmnos:content attribute="item" id="ReceiveItemContent" key="Item">
		</bpmnos:content>
		<bpmnos:content attribute="size" id="ReceiveSizeContent" key="Size">
		</bpmnos:content>
	</bpmnos:message>
</bpmn2:extensionElements>
```
