//const URL_BASE = 'http://alpakagott:42069/scripts/csvdata.php?days=';
const URL_BASE = 'http://alpakagott/heizung/csvdata.php?days=';
//const URL_SINGLE='http://alpakagott:42069/scripts/testdata.csv';
const SINGLE_BO = "csvBuff24";

var g0;

function setVisibilityG0(e1){
	g0.setVisibility(e1.id,e1.checked);
}

function enableRadioButtons(enable){
	const rbs = document.querySelectorAll('input[name="allduration"]');
	for(const rb of rbs){
		rb.disabled = !enable;
	}
}

function getDailyData(days){
	enableRadioButtons(false);
	document.getElementById("MainGraph").innerHTML = "";
	var b = document.getElementsByClassName("graph");
	for(a = 0; a < b.length; a++){
		b[a].innerHTML += '<div class="multi-spinner-container"><div class="multi-spinner"><div class="multi-spinner"><div class="multi-spinner"><div class="multi-spinner"><div class="multi-spinner"><div class="multi-spinner"></div></div></div></div></div></div></div>';
	}

	const http = new XMLHttpRequest();
	http.open("GET", URL_BASE + days);
	http.send();

	document.getElementById(SINGLE_BO).innerHTML = "";
	http.onload = (e) => {
		console.log(http.responseText)
		document.getElementById(SINGLE_BO).innerHTML += http.responseText;
	}

	http.onloadend = (e) => {
		console.log("done!");
		g0 = new Dygraph(

			// containing div
			document.getElementById("MainGraph"),
			document.getElementById(SINGLE_BO).innerHTML,
			{
				showRangeSelector: true,
				rangeSelectorPlotFillColor: '#E15D44',
				rangeSelectorPlotFillGradientColor: 'tomato',
				colorValue: 0.9,
				fillAlpha: 0.4,
				title:"",
				ylabel:"Temperaturen in °C",
				xlabel:"Zeit",
		
				//rollPeriod: 0.5,
			  
			  //errorBars: true,
			  
			  valueRange: [0,100],
			  series:{
				red:{
					color: "red",
					strokeWidth: 1,
					drawPoints: false,
				},
				blue:{
					color: "blue",
					strokeWidth: 1,
					drawPoints: false,
				},
				green:{
					color: "green",
					strokeWidth: 1,
					drawPoints: false,
				},
				yellow:{
					color: "#e8be00",
					strokeWidth: 1,
					drawPoints: false,
				},
				brown:{
					color: "brown",
					strokeWidth: 1,
					drawPoints: false,
				},
				room:{
					color: "purple",
					strokeWidth: 1,
					drawPoints: false,
				},
				white:{
					color: "black",
					strokeWidth: 1,
					drawPoints: false,
				},
			  }
			}
		
		  );
		  g0.setVisibility(2,false);
		  g0.setVisibility(3,false);
		  g0.setVisibility(6,false);
		  enableRadioButtons(true);
	}
}

function clickuptate(){
	const rbs = document.querySelectorAll('input[name="allduration"]');
	for(const rb of rbs){
		if(rb.checked){
			console.log("Selected: " + rb.value);
			getDailyData(rb.value);
		}
	}
}

document.getElementById("temptimesel").onchange = clickuptate;

getDailyData(1);