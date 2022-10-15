<?php

	// ________                         __  .__               
	// \______ \   ____   _____   _____/  |_|__| ____ ________
	//  |    |  \ /  _ \ /     \ /  _ \   __\  |/ ___\\___   /
	//  |    `   (  <_> )  Y Y  (  <_> )  | |  \  \___ /    / 
	// /_______  /\____/|__|_|  /\____/|__| |__|\___  >_____ \
	//         \/             \/                    \/      \/

	// Include libraries to access ThingSpeak platform
	include_once "tsFramework.php";
	
	// Setup section	
	date_default_timezone_set('Europe/Brussels');
	// ThingSpeak Channel ID for retrienving data 
	$channelID="<channel_id>";
  	// ThingSpeak read API key
  	$apiKey="<read_api_key>";
	$sensorIDX=<domoticz_sensor_idx>;
	$domoticzIP="<domoticz_server_ip>";

	// Run this script every minutes to have minute resolution in the graph.
	// * * * * * /home/pi/domoticz/scripts/php/SolarUpdate.sh
	
	/*
		pi@domoticz:~/domoticz/scripts/php $ cat SolarUpdate.sh
		#!/bin/bash
		cd /home/pi/domoticz/scripts/php
		php tsDomoticz.php
	*/
	
	function sendToSensor($domoticzIP,$sensorIDX,$val)
	{
		// API Reference : https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s#Electricity_.28instant_and_counter.29

		$API=$domoticzIP;

   		$curl = curl_init();
		curl_setopt($curl, CURLOPT_CUSTOMREQUEST, "POST");
   		
		curl_setopt($curl, CURLOPT_POST, 1);
		curl_setopt($curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_setopt($curl, CURLOPT_HTTPHEADER, array(
							'Accept: application/json, text/plain, */*',
							'Content-Type: application/json;charset=utf-8'
							));
		curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($curl, CURLOPT_SSL_VERIFYHOST, false);
		curl_setopt($curl, CURLOPT_URL, "https://".$API."/json.htm?type=command&param=udevice&idx=".$sensorIDX."&nvalue=0&svalue=".$val.";0");
		curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);

		$result = curl_exec($curl);
		curl_close($curl);

		if(json_decode($result)->status=="OK") return TRUE;
		else return FALSE;
	}

	// =====================================
	// Entry Point
	// =====================================
	
	print("\nFetching data from ThingSpeak...");
	// 1 Read data from ThingSpeak
	$tsData=tsGetLastFieldData($apiKey,$channelID,2);
	print("\n");

	print("Writing data to Domoticz sensor...");
	// 2 Write data to Domoticz
	sendToSensor($domoticzIP,$sensorIDX,$tsData);
	print("\n");
	print($tsData." sent to Domoticz \n");
	
?>
