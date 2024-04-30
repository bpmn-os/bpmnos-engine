@page assignment_problem Assignment problem
# Collaboration `Assignment_problem`
The *Assignment problem* can be described as the problem of finding an assignment of clients to servers, such that 

- each client is assigned to exactly one server,
- exactly one client is assigned to each server, 

and the total cost that of all assignment is minimised.
## Diagram
\htmlonly
<object data="./Assignment_problem.svg" type="image/svg+xml" style="max-width: 100%;">Assignment_problem</object>
\endhtmlonly


## Process `ClientProcess`
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


## Task `SendRequestTask`
The task creates a message containing the instance identifier of the client and waits until the message is delivered to a server.
```xml
<bpmn2:extensionElements>
	<bpmnos:messages>
		<bpmnos:message name="Message">
			<bpmnos:content attribute="instance" id="Content_3t5lqtk" key="ClientId">
			</bpmnos:content>
		</bpmnos:message>
	</bpmnos:messages>
</bpmn2:extensionElements>
```


## Process `ServerProcess`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" value="0">
			</bpmnos:attribute>
			<bpmnos:attribute id="ClientAttribute" name="client" type="string">
			</bpmnos:attribute>
		</bpmnos:attributes>
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


## Task `ReceiveRequestTask`
The task waits for a message from a client. When the message is received, it looks up the cost of the assignment in the table `costs.csv` and sets the `cost` attribute accordingly.
```xml
<bpmn2:extensionElements>
	<bpmnos:messages>
		<bpmnos:message name="Message">
			<bpmnos:content attribute="client" id="Content_22jl468" key="ClientId">
			</bpmnos:content>
		</bpmnos:message>
	</bpmnos:messages>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Attribute_20a021k" name="cost" objective="minimize" type="decimal" value="0" weight="1">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="cost" id="Operator_2ldkdhd" type="lookup">
				<bpmnos:parameter name="source" value="file">
				</bpmnos:parameter>
				<bpmnos:parameter name="filename" value="costs.csv">
				</bpmnos:parameter>
				<bpmnos:parameter name="key" value="Costs">
				</bpmnos:parameter>
				<bpmnos:parameter attribute="client" name="ClientId">
				</bpmnos:parameter>
				<bpmnos:parameter attribute="instance" name="ServerId">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```
