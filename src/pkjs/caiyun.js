var secrets = require('./secrets');
var weatherCommon = require('./weather');
var WEATHER_CONDITION=null;

module.exports.getWeatherFromCoords = getWeatherFromCoords;
module.exports.getForecastFromCoords = getForecastFromCoords;

function getWeatherCode(conditionCode){
	if(!WEATHER_CONDITION){
		WEATHER_CONDITION={
			CLEAR_DAY:weatherCommon.icons.CLEAR_DAY,//0 Sunny
			CLEAR_NIGHT:weatherCommon.icons.CLEAR_NIGHT,//1 Clear
			PARTLY_CLOUDY_DAY:weatherCommon.icons.PARTLY_CLOUDY,//2 Fair
			PARTLY_CLOUDY_NIGHT:weatherCommon.icons.PARTLY_CLOUDY_NIGHT,//3 Fair
			CLOUDY:weatherCommon.icons.CLOUDY_DAY,//4 Cloudy
			RAIN:weatherCommon.icons.LIGHT_RAIN,
		  SNOW:weatherCommon.icons.HEAVY_SNOW
		};
	}
	return WEATHER_CONDITION[conditionCode];
}

function getWeatherFromCoords(pos) {
  // Construct URL https://api.thinkpage.cn/v3/weather/now.json?key=hcgix3wepfnwqtpl&location=beijing&language=zh-Hans&unit=c

  var url = 'https://api.caiyunapp.com/v2/' +secrets.CAI_YUN_APP_ID+'/'+pos.coords.longitude+','+ pos.coords.latitude+'/realtime.json';
  console.log(url);
  getAndSendCurrentWeather(url);
}


function getForecastFromCoords(pos) {
  var forecastURL = 'https://api.caiyunapp.com/v2/' +secrets.CAI_YUN_APP_ID+'/'+pos.coords.longitude+','+ pos.coords.latitude+'/forecast.json';

  getAndSendWeatherForecast(forecastURL);
}

// accepts an url, gets weather data from it, and sends it to the watch
function getAndSendCurrentWeather(url) {
  weatherCommon.xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      if(json.result&&json.result.status=='ok') {
        var temperature = Math.round(json.result.temperature);
        console.log('Temperature is ' + temperature);
        // Conditions
        var conditionCode = json.result.skycon;
        console.log('Condition code is ' + conditionCode);
        var iconToLoad = getWeatherCode(conditionCode);
        if(conditionCode=='RAIN'){
          if(json.result.precipitation.local.intensity>0.25){
            iconToLoad=weatherCommon.icons.HEAVY_RAIN;  
          }    
        }

        // Assemble dictionary using our keys
        var dictionary = {
          'KEY_TEMPERATURE': temperature,
          'KEY_CONDITION_CODE': iconToLoad
        };

        console.log(JSON.stringify(dictionary));

        weatherCommon.sendWeatherToPebble(dictionary);
      }
  });
}

function getAndSendWeatherForecast(url) {
  console.log(url);
  weatherCommon.xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      if(json.result.daily&&json.result.daily.status=='ok') {
      	var weatherInfo=json.result.daily;
        console.log(JSON.stringify(weatherInfo));
      	// Assemble dictionary using our keys
        var conditionCode = weatherInfo.skycon[0].value;
        var iconToLoad = getWeatherCode(conditionCode);
        if(conditionCode=='RAIN'){
          if(weatherInfo.precipitation[0].value>0.25){
            iconToLoad=weatherCommon.icons.HEAVY_RAIN;  
          }    
        }
        var high=weatherInfo.temperature[0].min;
        var low=weatherInfo.temperature[0].max;
        var dictionary = {
          'KEY_FORECAST_CONDITION': iconToLoad,
          'KEY_FORECAST_TEMP_HIGH': high,
          'KEY_FORECAST_TEMP_LOW': low
        };

        console.log(JSON.stringify(dictionary));

        weatherCommon.sendWeatherToPebble(dictionary);
      }
      
  });
}