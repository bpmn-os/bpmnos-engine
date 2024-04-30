@page guided_bin_packing_problem Bin packing problem (guided)
# Collaboration `Bin_packing_problem`
Given a set of items, each having a specifed size, and a set of bins each having a specified capacity, the *Bin packing problem* can be described as the problem of finding the minimal number of bins required to include all items such that the total size of all items associated to a bin does not exceed the capacity of the bin.

The model contains guidance on the allocation of items to bins. 
## Diagram
\htmlonly
<object data="./Guided_bin_packing_problem.svg" type="image/svg+xml" style="max-width: 100%;">Bin_packing_problem</object>
\endhtmlonly


```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Items" name="items" type="integer">
		</bpmnos:attribute>
		<bpmnos:attribute id="ItemRequests" name="item_requests" type="integer" value="0">
		</bpmnos:attribute>
		<bpmnos:attribute id="Bins" name="bins" type="integer">
		</bpmnos:attribute>
		<bpmnos:attribute id="AvailableBins" name="available_bins" type="integer" value="0">
		</bpmnos:attribute>
		<bpmnos:attribute id="ItemsIncluded" name="items_included" type="integer" value="0">
		</bpmnos:attribute>
		<bpmnos:attribute id="Blocker" name="blocker" type="boolean" value="false">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```

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


## Task `RequestActivity`
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
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="item_requests" id="Operator_0pjma63" type="expression">
				<bpmnos:parameter name="linear" value="item_requests + 1">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
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


## Task `WaitActivity`
```xml
<bpmn2:extensionElements>
	<bpmnos:guidance type="exit">
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_2cloof7" scope="exit" type="decimal">
				<bpmnos:parameter name="linear" value="items_included == items">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
	</bpmnos:guidance>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="available_bins" id="Operator_0oi8qea" type="expression">
				<bpmnos:parameter name="linear" value="available_bins + 1">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
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
		<bpmnos:restrictions>
			<bpmnos:restriction id="RestrictionBlocker" scope="entry">
				<bpmnos:parameter name="linear" value="blocker == false">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:operators>
			<bpmnos:operator attribute="blocker" id="OperatorBlocker" type="assign">
				<bpmnos:parameter name="assign" value="true">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Ad-hoc SubProcess `AdHocSubProcess`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `IncludeItemActivity`
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
			<bpmnos:operator attribute="items_included" id="OperatorItemsIncluded" type="expression">
				<bpmnos:parameter name="linear" value="items_included + 1">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="blocker" id="OperatorUnblock" type="assign">
				<bpmnos:parameter name="assign" value="false">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Event `CatchRequestMessage`
The guidance for the allocation of items to bins requires that all items have sent their request, all bins are available, and that the inclusion of an item will not exceed the capacity. Allocations are made in an order that the item which minimise the residual capacity is chosen first.
```xml
<bpmn2:extensionElements>
	<bpmnos:message name="Request">
		<bpmnos:content attribute="item" id="ReceiveItemContent" key="Item">
		</bpmnos:content>
		<bpmnos:content attribute="size" id="ReceiveSizeContent" key="Size">
		</bpmnos:content>
	</bpmnos:message>
	<bpmnos:guidance type="message">
		<bpmnos:attributes>
			<bpmnos:attribute id="Residual" name="residual" objective="minimize" type="decimal" weight="1">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_0ve1mi3" scope="exit" type="decimal">
				<bpmnos:parameter name="linear" value="item_requests == items">
				</bpmnos:parameter>
			</bpmnos:restriction>
			<bpmnos:restriction id="Restriction_247eq94" scope="exit" type="decimal">
				<bpmnos:parameter name="linear" value="available_bins == bins">
				</bpmnos:parameter>
			</bpmnos:restriction>
			<bpmnos:restriction id="RestrictionSize" scope="exit" type="decimal">
				<bpmnos:parameter name="linear" value="total_size + size <= capacity">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:operators>
			<bpmnos:operator attribute="residual" id="OperatorResidual" type="expression">
				<bpmnos:parameter name="linear" value="capacity + 2 - total_size - size">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:guidance>
</bpmn2:extensionElements>
```
