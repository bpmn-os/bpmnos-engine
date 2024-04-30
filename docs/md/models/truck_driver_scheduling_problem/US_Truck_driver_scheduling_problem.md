@page truck_driver_scheduling_problem Truck driver scheduling problem
# Process `TruckDriverProcess`

## Diagram
\htmlonly
<object data="./US_Truck_driver_scheduling_problem.svg" type="image/svg+xml" style="max-width: 100%;">TruckDriverProcess</object>
\endhtmlonly


```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="Timestamp" name="timestamp" objective="minimize" type="decimal" value="0" weight="1">
			</bpmnos:attribute>
			<bpmnos:attribute id="EarliestVisits" name="earliest_visits" type="collection">
			</bpmnos:attribute>
			<bpmnos:attribute id="LatestVisits" name="latest_visits" type="collection">
			</bpmnos:attribute>
			<bpmnos:attribute id="TravelTimes" name="travel_times" type="collection">
			</bpmnos:attribute>
			<bpmnos:attribute id="ServiceTimes" name="service_times" type="collection">
			</bpmnos:attribute>
			<bpmnos:attribute id="LastRest" name="last_rest" type="decimal" value="0">
			</bpmnos:attribute>
			<bpmnos:attribute id="LastBreak" name="last_break" type="decimal" value="0">
			</bpmnos:attribute>
			<bpmnos:attribute id="DrivingSinceRest" name="driving_since_rest" type="decimal" value="0">
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


## DataObject `DataObject_09vd7xw`
```xml
<bpmn2:extensionElements>
	<bpmnos:attributes>
		<bpmnos:attribute id="MaxDrivingSinceRest" name="max_driving_since_rest" type="decimal" value="660">
		</bpmnos:attribute>
		<bpmnos:attribute id="MaxElapsedSinceRest" name="max_elapsed_since_rest" type="decimal" value="840">
		</bpmnos:attribute>
		<bpmnos:attribute id="MaxElapsedSinceBreak" name="max_elapsed_since_break" type="decimal" value="480">
		</bpmnos:attribute>
		<bpmnos:attribute id="MinRestDuration" name="min_rest_duration" type="decimal" value="600">
		</bpmnos:attribute>
		<bpmnos:attribute id="MinBreakDuration" name="min_break_duration" type="decimal" value="30">
		</bpmnos:attribute>
	</bpmnos:attributes>
</bpmn2:extensionElements>
```


## SubProcess `CustomerTripActivity`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="ServiceTime" name="service_time" type="decimal">
				<bpmnos:parameter attribute="service_times" name="collection">
				</bpmnos:parameter>
			</bpmnos:attribute>
			<bpmnos:attribute id="TravelTime" name="travel_time" type="decimal">
				<bpmnos:parameter name="collection" value="travel_times">
				</bpmnos:parameter>
			</bpmnos:attribute>
			<bpmnos:attribute id="LatestVisit" name="latest_visit" type="decimal">
				<bpmnos:parameter attribute="latest_visits" name="collection">
				</bpmnos:parameter>
			</bpmnos:attribute>
			<bpmnos:attribute id="EarliestVisit" name="earliest_visit" type="decimal">
				<bpmnos:parameter attribute="earliest_visits" name="collection">
				</bpmnos:parameter>
			</bpmnos:attribute>
			<bpmnos:attribute id="DrivingTime" name="remaining_driving_time" type="decimal">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="remaining_driving_time" id="Operator_03lemss" type="assign">
				<bpmnos:parameter attribute="travel_time" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `DriveTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="DriveDuration" name="duration" type="decimal">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="duration" id="Operator_14p2k9c" type="expression">
				<bpmnos:parameter name="generic" value="min( remaining_driving_time, max_driving_since_rest - driving_since_rest, max_elapsed_since_rest - (timestamp - last_rest ), max_elapsed_since_break - (timestamp - last_break) )">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="timestamp" id="Operator_2lnpndi" type="expression">
				<bpmnos:parameter name="linear" value="timestamp + duration">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="driving_since_rest" id="Operator_2b2lsd5" type="expression">
				<bpmnos:parameter name="linear" value="driving_since_rest + duration">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="remaining_driving_time" id="Operator_1ub09t4" type="expression">
				<bpmnos:parameter name="linear" value="remaining_driving_time - duration">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `VisitTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_25slc8t" scope="entry">
				<bpmnos:parameter name="linear" value="timestamp <= latest_visit">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:operators>
			<bpmnos:operator attribute="timestamp" id="Operator_36e9gct" type="expression">
				<bpmnos:parameter name="linear" value="timestamp + service_time">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_1uszjtx`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_19mu7pk">
			<bpmnos:parameter name="linear" value="remaining_driving_time == 0">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_0s8gidz`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_09erveo">
			<bpmnos:parameter name="linear" value="remaining_driving_time > 0">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## SubProcess `OffDutyActivity`
\htmlonly
<object data="US_Truck_driver_scheduling_problem-OffDutyActivity.svg" type="image/svg+xml" style="max-width: 100%;">OffDutyActivity</object>
\endhtmlonly


```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="OffDutyDuration" name="duration" type="decimal">
			</bpmnos:attribute>
			<bpmnos:attribute id="OffDutyType" name="offduty_type" type="string">
			</bpmnos:attribute>
		</bpmnos:attributes>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `DecideDuringTripTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_OffDutyType" scope="exit">
				<bpmnos:parameter name="enumeration" value="offduty_type in ["break","rest"]">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:decisions>
			<bpmnos:decision attribute="offduty_type" id="Decision_2hfddnu">
			</bpmnos:decision>
		</bpmnos:decisions>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_00x8mrp`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_BreakOnTrip">
			<bpmnos:parameter name="string" value="offduty_type == "break"">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_16bmg1w`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_RestOnTrip">
			<bpmnos:parameter name="linear" value="offduty_type == "rest"">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## Task `BreakDuringTripTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="timestamp" id="Operator_3midv9s" type="expression">
				<bpmnos:parameter name="linear" value="timestamp + duration">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="last_break" id="Operator_2g12etq" type="assign">
				<bpmnos:parameter attribute="timestamp" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `RestDuringTripTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="timestamp" id="Operator_3fkas66" type="expression">
				<bpmnos:parameter name="linear" value="timestamp + duration">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="last_rest" id="Operator_0d2otbv" type="assign">
				<bpmnos:parameter attribute="timestamp" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="last_break" id="Operator_3fj94i1" type="assign">
				<bpmnos:parameter attribute="timestamp" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `Activity_0r4ynpt`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_3jc9q6n" scope="exit">
				<bpmnos:parameter name="linear" value="duration >= min_break_duration">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:decisions>
			<bpmnos:decision attribute="duration" id="Decision_02uohv2">
			</bpmnos:decision>
		</bpmnos:decisions>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `Activity_1yum9rm`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_3ntmj4i" scope="exit">
				<bpmnos:parameter name="linear" value="duration >= min_rest_duration">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:decisions>
			<bpmnos:decision attribute="duration" id="Decision_0395q7p">
			</bpmnos:decision>
		</bpmnos:decisions>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## SubProcess `WaitActivity`
\htmlonly
<object data="US_Truck_driver_scheduling_problem-WaitActivity.svg" type="image/svg+xml" style="max-width: 100%;">WaitActivity</object>
\endhtmlonly


```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:attributes>
			<bpmnos:attribute id="WaitDuration" name="duration" type="decimal">
			</bpmnos:attribute>
			<bpmnos:attribute id="WaitType" name="wait_type" type="string">
			</bpmnos:attribute>
		</bpmnos:attributes>
		<bpmnos:operators>
			<bpmnos:operator attribute="max_duration" id="Operator_032bbi4" type="expression">
				<bpmnos:parameter name="generic" value="max( earliest_visit - timestamp, max_rest_duration )">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `BreakAtDestinationTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="timestamp" id="Operator_2ih0qpu" type="expression">
				<bpmnos:parameter name="linear" value="timestamp + duration">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="last_break" id="Operator_1v9kqtd" type="assign">
				<bpmnos:parameter attribute="timestamp" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `RestAtDestinationTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:operators>
			<bpmnos:operator attribute="timestamp" id="Operator_0tuoo7c" type="expression">
				<bpmnos:parameter name="linear" value="timestamp + duration">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="last_rest" id="Operator_2vvafdb" type="assign">
				<bpmnos:parameter attribute="timestamp" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
			<bpmnos:operator attribute="last_break" id="Operator_1frp5g3" type="assign">
				<bpmnos:parameter attribute="timestamp" name="assign">
				</bpmnos:parameter>
			</bpmnos:operator>
		</bpmnos:operators>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `DecideAtDestinationTask`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_0uj7fj8" scope="exit">
				<bpmnos:parameter name="enumeration" value="wait_type in ["wait", "break", "rest"]">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:decisions>
			<bpmnos:decision attribute="wait_type" id="Decision_0udt1qg">
			</bpmnos:decision>
		</bpmnos:decisions>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_1at46ok`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_Wait">
			<bpmnos:parameter name="string" value="wait_type == "wait"">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_08rnlb8`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_Break">
			<bpmnos:parameter name="string" value="wait_type == "break"">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_08pr2zc`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_Rest">
			<bpmnos:parameter name="string" value="wait_type == "rest"">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## Task `Activity_0ukjrfm`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_3e1anpv" scope="exit">
				<bpmnos:parameter name="linear" value="duration >= min_break_duration">
				</bpmnos:parameter>
			</bpmnos:restriction>
			<bpmnos:restriction id="Restriction_1inerld" scope="exit">
				<bpmnos:parameter name="linear" value="timestamp + duration >=  earliest_visit">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:decisions>
			<bpmnos:decision attribute="duration" id="Decision_1k9jgn5">
			</bpmnos:decision>
		</bpmnos:decisions>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## Task `Activity_0evasg5`
```xml
<bpmn2:extensionElements>
	<bpmnos:status>
		<bpmnos:restrictions>
			<bpmnos:restriction id="Restriction_1cmsb7s" scope="exit">
				<bpmnos:parameter name="linear" value="duration >= min_rest_duration">
				</bpmnos:parameter>
			</bpmnos:restriction>
			<bpmnos:restriction id="Restriction_29ta0nq">
				<bpmnos:parameter name="linear" value="timestamp + duration >=  earliest_visit">
				</bpmnos:parameter>
			</bpmnos:restriction>
		</bpmnos:restrictions>
		<bpmnos:decisions>
			<bpmnos:decision attribute="duration" id="Decision_0cnh1uk">
			</bpmnos:decision>
		</bpmnos:decisions>
	</bpmnos:status>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_12gilga`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_1a83q9b">
			<bpmnos:parameter name="linear" value="timestamp < earliest_visit">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```


## SequenceFlow `Flow_14ovnm9`
```xml
<bpmn2:extensionElements>
	<bpmnos:restrictions>
		<bpmnos:restriction id="Restriction_3lfp9vl">
			<bpmnos:parameter name="linear" value="timestamp >= earliest_visit">
			</bpmnos:parameter>
		</bpmnos:restriction>
	</bpmnos:restrictions>
</bpmn2:extensionElements>
```
