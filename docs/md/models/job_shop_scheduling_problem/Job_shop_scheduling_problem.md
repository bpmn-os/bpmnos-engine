@page job_shop_scheduling_problem Job shop scheduling problem
# Collaboration `Job_shop_scheduling_problem`

## Diagram
\htmlonly
<object data="./Job_shop_scheduling_problem.svg" type="image/svg+xml" style="max-width: 100%;">Job_shop_scheduling_problem</object>
\endhtmlonly


```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Makespan" name="makespan" objective="minimize" type="decimal" value="0" weight="1">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```

## Process `OrderProcess`
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


## DataObject `DataObject_1xt50y3`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Instance" name="instance" type="string">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```


## DataObject `DataObject_1vrduku`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Machines" name="machines" type="collection">
		</bpmnos:attribute>
		<bpmnos:attribute id="Durations" name="durations" type="collection">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```


## SubProcess `JobActivity`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Machine" name="machine" type="string">
				<bpmnos:parameter attribute="machines" name="collection">
				</bpmnos:parameter>
			</bpmnos:attribute>
			<bpmnos:attribute id="Duration" name="duration" type="decimal">
				<bpmnos:parameter attribute="durations" name="collection">
				</bpmnos:parameter>
			</bpmnos:attribute>
		</bpmnos:attributes>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `SendJobTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:messages>
		<bpmnos:message name="Request">
			<bpmnos:parameter attribute="machine" name="machine">
			</bpmnos:parameter>
			<bpmnos:content attribute="instance" id="Content_1485jek" key="Order">
			</bpmnos:content>
			<bpmnos:content attribute="duration" id="Content_2d4hb1c" key="Duration">
			</bpmnos:content>
		</bpmnos:message>
	</bpmnos:messages>
</bpmn2:extensionElements>
```


## Task `NoticeJobCompletionTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="makespan" id="OperatorMakespan" type="expression">
				<bpmnos:parameter name="linear" value="timestamp">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
	<bpmnos:messages>
		<bpmnos:message name="Completion">
		</bpmnos:message>
	</bpmnos:messages>
</bpmn2:extensionElements>
```


## Process `MachineProcess`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" value="0">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="machine" id="OperatorMachine" type="assign">
				<bpmnos:parameter attribute="instance" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
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


## DataObject `DataObject_0ukass4`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="Jobs" name="jobs" type="integer">
		</bpmnos:attribute>
		<bpmnos:attribute id="JobRequests" name="job_requests" type="integer" value="0">
		</bpmnos:attribute>
		<bpmnos:attribute id="Machine" name="machine" type="string">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```


## Task `WaitActivity`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_1bttfjg" scope="exit">
				<bpmnos:parameter name="linear" value="job_requests == jobs">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Event-SubProcess `EventSubProcess`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="JobDuration" name="duration" type="decimal">
			</bpmnos:attribute>
			<bpmnos:attribute id="JobOrder" name="order" type="string">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="job_requests" id="OperatorJobRequests" type="expression">
				<bpmnos:parameter name="linear" value="job_requests + 1">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `ConductJobTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="timestamp" id="Operator_3pf5m16" type="expression">
				<bpmnos:parameter name="linear" value="timestamp + duration">
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
		<bpmnos:parameter attribute="machine" name="machine">
		</bpmnos:parameter>
		<bpmnos:content attribute="duration" id="Content_3gvltfq" key="Duration">
		</bpmnos:content>
		<bpmnos:content attribute="order" id="Content_331q4tt" key="Order">
		</bpmnos:content>
	</bpmnos:message>
</bpmn2:extensionElements>
```


## Event `ThrowCompletionMessage`
```xml
<bpmn2:extensionElements>
	<bpmnos:message name="Completion">
		<bpmnos:parameter attribute="order" name="recipient">
		</bpmnos:parameter>
	</bpmnos:message>
</bpmn2:extensionElements>
```
