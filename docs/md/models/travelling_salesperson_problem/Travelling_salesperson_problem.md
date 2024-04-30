@page travelling_salesperson_problem Travelling salesperson problem
# Process `TravellingSalesperson_Process`

## Diagram
\htmlonly
<object data="./Travelling_salesperson_problem.svg" type="image/svg+xml" style="max-width: 100%;">TravellingSalesperson_Process</object>
\endhtmlonly


```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Origin" name="origin" type="string">
			</bpmnos:attribute>
			<bpmnos:attribute id="Locations" name="locations" type="collection">
			</bpmnos:attribute>
			<bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" value="0">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="current_location" id="Operator_2tvimj1" type="assign">
				<bpmnos:parameter attribute="origin" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## DataObject `DataObject_0jj2j83`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="CurrentLocation" name="current_location" type="string">
		</bpmnos:attribute>
	</bpmnos:attributes>
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


## Task `VisitLocation`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Location" name="location" type="string">
				<bpmnos:parameter attribute="locations" name="collection">
				</bpmnos:parameter>
			</bpmnos:attribute>
			<bpmnos:attribute id="Distance" name="distance" objective="minimize" type="decimal" weight="1">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="distance" id="Operator_0sdnaip" type="lookup">
				<bpmnos:parameter name="source" value="file">
				</bpmnos:parameter>
				<bpmnos:parameter name="filename" value="distances.csv">
				</bpmnos:parameter>
				<bpmnos:parameter name="key" value="Distance">
				</bpmnos:parameter>
				<bpmnos:parameter attribute="current_location" name="From">
				</bpmnos:parameter>
				<bpmnos:parameter attribute="location" name="To">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="current_location" id="Operator_06serru" type="assign">
				<bpmnos:parameter attribute="location" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `ReturnTrip`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="ReturnDistance" name="distance" objective="minimize" type="decimal" weight="1">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="distance" id="Operator_02ck1sv" type="lookup">
				<bpmnos:parameter name="source" value="file">
				</bpmnos:parameter>
				<bpmnos:parameter name="filename" value="distances.csv">
				</bpmnos:parameter>
				<bpmnos:parameter name="key" value="Distance">
				</bpmnos:parameter>
				<bpmnos:parameter attribute="current_location" name="From">
				</bpmnos:parameter>
				<bpmnos:parameter attribute="origin" name="To">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="current_location" id="Operator_1q37okk" type="assign">
				<bpmnos:parameter attribute="origin" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```
