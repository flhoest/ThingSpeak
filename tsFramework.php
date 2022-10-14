<?php

	// ___________.__     .__            ____   _________                         __     
	// \__    ___/|  |__  |__|  ____    / ___\ /   _____/______    ____  _____   |  | __ 
	//   |    |   |  |  \ |  | /    \  / /_/  >\_____  \ \____ \ _/ __ \ \__  \  |  |/ / 
	//   |    |   |   Y  \|  ||   |  \ \___  / /        \|  |_> >\  ___/  / __ \_|    <  
	//   |____|   |___|  /|__||___|  //_____/ /_______  /|   __/  \___  >(____  /|__|_ \ 
	//                 \/          \/                 \/ |__|         \/      \/      \/ 
	//    (c) 2022 Frederic Lhoest    v 0.1                                                                   

	// Set of REST API functions to interact with ThingSpeak IoT platform

	// ------------------------------------------------------------------
	// Function retrieving the last 24 hours of data for a specific channel ID. 
	// Note : all fields returned into a JSON string
	// ------------------------------------------------------------------

	function tsGetLast24Data($apiKey,$channelID)
	{
		$startOfDay=date("Y-m-d%2000:00:00");
	    	$API_URL="api.thingspeak.com/channels/".$channelID."/feeds.json?api_key=".$apiKey."&start=".$startOfDay."&timezone=Europe/Brussels";
	     
		$curl = curl_init();
		curl_setopt($curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_setopt($curl, CURLOPT_HTTPHEADER, array(
							"Authorization: Bearer ".$apiKey,
							'Accept: application/json, text/plain, */*',
							'Content-Type: application/json;charset=utf-8'
							));
		curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($curl, CURLOPT_SSL_VERIFYHOST, false);
		curl_setopt($curl, CURLOPT_URL, "https://".$API_URL);

		curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
		$result = curl_exec($curl);
		curl_close($curl);
		usleeep(50*1000);
		return json_decode($result);	
	}

	// ------------------------------------------------------------------
	// Function retrieving the last value collected for a specific field.
	// ------------------------------------------------------------------

	function tsGetLastFieldData($apiKey,$channelID,$fieldID)
	{
	    	$API_URL="api.thingspeak.com/channels/".$channelID."/fields/".$fieldID.".json?api_key=".$apiKey."&results=1&timezone=Europe/Brussels";
	    	$curl = curl_init();
		curl_setopt($curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_setopt($curl, CURLOPT_HTTPHEADER, array(
							"Authorization: Bearer ".$apiKey,
							'Accept: application/json, text/plain, */*',
							'Content-Type: application/json;charset=utf-8'
							));
		curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($curl, CURLOPT_SSL_VERIFYHOST, false);
		curl_setopt($curl, CURLOPT_URL, "https://".$API_URL);

		curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
		$result = curl_exec($curl);
		curl_close($curl);
		usleeep(50*1000);
		return trim(json_decode($result)->feeds[0]->field2);	
	}
		
?>
