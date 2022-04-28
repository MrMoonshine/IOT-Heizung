const STATE_URL_BASE = "http://alpakagott/heizung/dbsta.php?"

function stateID(state){
    if(state){
        return "1";
    }else{
        return "0";
    }
}

var pumpform = document.getElementById("pumpform");

var heizpumpe = document.getElementById("heizpumpe");
var bufferpumpe = document.getElementById("bufferpumpe");
var warmepumpe = document.getElementById("warmepumpe");

var autosloar = document.getElementById("autosolar");
var solarpumpe = document.getElementById("solarpumpe");
// DIsable Solarpump selection
solarpumpe.disabled = this.checked;

autosloar.addEventListener("change", function(){
    solarpumpe.disabled = this.checked;
});
var singles = [heizpumpe, bufferpumpe, solarpumpe];
//Fetch all states
const xhrsolarpumpe = new XMLHttpRequest();
xhrsolarpumpe.open('GET', STATE_URL_BASE + "pump=2", true);
xhrsolarpumpe.onload = function(){
    autosloar.checked = xhrsolarpumpe.responseText.includes("AUTO");
    if(!autosloar.checked){
        solarpumpe.checked = xhrsolarpumpe.responseText.includes("ON");
    }else{
        solarpumpe.disabled = true;
    }
}
xhrsolarpumpe.send();

const xhrwp = new XMLHttpRequest();
xhrwp.open('GET', STATE_URL_BASE + "pump=" + warmepumpe.value, true);
xhrwp.onload = function(){
    warmepumpe.checked = xhrwp.responseText.includes("ON");
}
xhrwp.send();

const xhrhp = new XMLHttpRequest();
xhrhp.open('GET', STATE_URL_BASE + "pump=" + heizpumpe.value, true);
xhrhp.onload = function(){
    heizpumpe.checked = xhrwp.responseText.includes("ON");
}
xhrhp.send();

const xhrbp = new XMLHttpRequest();
xhrbp.open('GET', STATE_URL_BASE + "pump=" + bufferpumpe.value, true);
xhrbp.onload = function(){
    bufferpumpe.checked = xhrwp.responseText.includes("ON");
}
xhrbp.send();

// Set all states
pumpform.onsubmit = function(){
    var urls = Array();
    singles.forEach(element => {
        if(!element.disabled){
            var url = STATE_URL_BASE + "pump=" + element.value + "&set=" + stateID(element.checked);
            urls.push(url);
        }
    });

    if(autosloar.checked){
        urls.push(STATE_URL_BASE + "pump=" + solarpumpe.value + "&set=2");
    }

    urls.push(STATE_URL_BASE + "pump=" + 19 + "&set=" + stateID(warmepumpe.checked));
    urls.push(STATE_URL_BASE + "pump=" + 18 + "&set=" + stateID(warmepumpe.checked));
    //Set states:
    urls.forEach(element => {
        const xhr = new XMLHttpRequest();
        xhr.open('GET', element, true);
        xhr.send();
    });
    alert("Zustände werden gesetzt!");
}
