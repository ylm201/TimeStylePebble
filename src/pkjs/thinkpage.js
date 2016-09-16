var secrets = require('./secrets');
var weatherCommon = require('./weather');
var WEATHER_CONDITION=null;

module.exports.getWeatherFromCoords = getWeatherFromCoords;
module.exports.getForecastFromCoords = getForecastFromCoords;

function getWeatherCode(conditionCode){
	if(!WEATHER_CONDITION){
		WEATHER_CONDITION=new Array(
			weatherCommon.icons.CLEAR_DAY,//0 Sunny
			weatherCommon.icons.CLEAR_NIGHT,//1 Clear
			weatherCommon.icons.CLEAR_DAY,//2 Fair
			weatherCommon.icons.CLEAR_NIGHT,//3 Fair
			weatherCommon.icons.CLOUDY_DAY,//4 Cloudy
			weatherCommon.icons.PARTLY_CLOUDY,//5 Partly Cloudy
			weatherCommon.icons.PARTLY_CLOUDY_NIGHT,//6 Partly Cloudy
			weatherCommon.icons.PARTLY_CLOUDY,//7 Mostly Cloudy
			weatherCommon.icons.PARTLY_CLOUDY_NIGHT,//8 Mostly Cloudy
			weatherCommon.icons.CLOUDY_DAY,//9 Overcast
			weatherCommon.icons.LIGHT_RAIN,//10 Shower
			weatherCommon.icons.LIGHT_RAIN,//11 Thundershower
			weatherCommon.icons.LIGHT_RAIN,//12 Thundershower with Hail
			weatherCommon.icons.LIGHT_RAIN,//13 Light Rain
			weatherCommon.icons.LIGHT_RAIN,//14 Moderate Rain
			weatherCommon.icons.HEAVY_RAIN,//15 Heavy Rain
			weatherCommon.icons.THUNDERSTORM,//16 Storm
			weatherCommon.icons.THUNDERSTORM,//17 Heavy Storm 
			weatherCommon.icons.THUNDERSTORM,//18 Severe Storm
			weatherCommon.icons.LIGHT_RAIN,//19 Ice Rain
			weatherCommon.icons.LIGHT_SNOW,//20 Sleet
			weatherCommon.icons.LIGHT_SNOW,//21 Snow Flurry 
			weatherCommon.icons.LIGHT_SNOW,//22 Light Snow
			weatherCommon.icons.LIGHT_SNOW,//23 Moderate Snow
			weatherCommon.icons.HEAVY_SNOW,//24 Moderate Snow
			weatherCommon.icons.HEAVY_SNOW//25 Snowstorm
		);
	}
	return WEATHER_CONDITION[conditionCode];
}

function getWeatherFromCoords(pos) {
  // Construct URL https://api.thinkpage.cn/v3/weather/now.json?key=hcgix3wepfnwqtpl&location=beijing&language=zh-Hans&unit=c

  var url = 'https://api.thinkpage.cn/v3/weather/now.json?location=' +
      pos.coords.latitude+':'+pos.coords.longitude + '&key=' + secrets.THINK_PAGE_APP_ID;
  console.log(url);
  getAndSendCurrentWeather(url);
}


function getForecastFromCoords(pos) {
  var forecastURL = 'https://api.thinkpage.cn/v3/weather/daily.json?location=' +
      pos.coords.latitude+':'+pos.coords.longitude +'&key=' + secrets.THINK_PAGE_APP_ID;

  getAndSendWeatherForecast(forecastURL);
}

// accepts an url, gets weather data from it, and sends it to the watch
function getAndSendCurrentWeather(url) {
  weatherCommon.xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      if(json.results) {
      	var weatherInfo=json.results[0];
        var temperature = Math.round(weatherInfo.now.temperature);
        console.log('Temperature is ' + temperature);
        // Conditions
        var conditionCode = weatherInfo.now.code;
        console.log('Condition code is ' + conditionCode);
        var iconToLoad = getWeatherCode(conditionCode);

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
      if(json.results) {
      	var weatherInfo=json.results[0];
      	// Assemble dictionary using our keys
        var dictionary = {
          'KEY_FORECAST_CONDITION': getWeatherCode(weatherInfo.daily[0].code_day),
          'KEY_FORECAST_TEMP_HIGH': parseInt(weatherInfo.daily[0].high),
          'KEY_FORECAST_TEMP_LOW': parseInt(weatherInfo.daily[0].low)
        };

        console.log(JSON.stringify(dictionary));

        weatherCommon.sendWeatherToPebble(dictionary);
      }
      
  });
}