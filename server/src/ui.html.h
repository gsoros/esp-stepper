const char indexHtmlTemplate[] PROGMEM = R"====(

<!DOCTYPE html>
<html>

<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body>
    <p id="message"></p>
    <div id="devices" style="text-align: left">
    </div>
    <script>
        var urlBase = "http://%s:%i/";
        var rate = 1000;
        var commands = [];
        var lastCommands = [];
        var nextCommands = [];
        var enabled = [];
        var types = [];
        var req = new XMLHttpRequest();

        function getConfig() {
            req.onreadystatechange = function () {
                if (this.readyState == 4) {
                    if (this.status == 200) {
                        var response = JSON.parse(this.responseText);
                        console.log(response);
                        if ('undefined' !== typeof response.name) {
                            message.innerHTML = "Connected to " + response.name;
                        }
                        if ('number' == typeof response.rate) {
                            rate = response.rate;
                            message.innerHTML += "<br>Command rate is " + 1000 / rate + "/s.";
                        }
                        if (0 < response.devices.length) {
                            response.devices.forEach(device => {
                                types[device.name] = device.type;
                                switch (device.type) {
                                    case "stepper":
                                        addStepper(device.name, device.commandMin, device.commandMax);
                                        break;
                                    case "led":
                                        addLed(device.name);
                                        break;
                                    default:
                                        message.innerHTML += device.name + ": unknown device type";
                                }
                            });
                        }
                    }
                    else {
                        message.innerHTML = 'Failed to get config, server replied: "'
                            + this.responseText + '".<br><button onClick="getConfig()">Retry</button>';
                    }
                }
                else {
                    message.innerHTML = "Getting config...";
                }
            };
            var url = urlBase + "api/config";
            console.log(url);
            req.open("GET", url);
            req.send();
        }
        getConfig();

        function addStepper(name, min, max) {
            var div = getDeviceDiv(name);
            div.appendChild(getDeviceEnabledSwitch(name));
            var output = document.createElement("div");
            output.innerHTML = "0";
            output.id = name + "_output";
            div.appendChild(output);
            var range = document.createElement("input");
            range.type = "range";
            range.style.margin = "0";
            range.style.width = "100%%";
            range.id = name;
            range.min = min;
            range.max = max;
            range.oninput = function () {
                commands[name] = this.value;
                document.getElementById(name + "_output").innerHTML = this.value;
                sendCommand(name);
            };
            div.appendChild(range);
            document.getElementById("devices").appendChild(div);
        }

        function addLed(name) {
            var div = getDeviceDiv(name);
            div.appendChild(getDeviceEnabledSwitch(name));
            document.getElementById("devices").appendChild(div);
        }

        function getDeviceDiv(name) {
            var h = document.createElement("h5");
            h.style.margin = "5px 0 5px";
            h.innerHTML = name;
            var div = document.createElement("div");
            div.style.border = "1px solid grey";
            div.style.margin = "10px";
            div.style.padding = "10px";
            div.style.borderRadius = "5px";
            div.appendChild(h);
            return div;
        }

        function getDeviceEnabledSwitch(name) {
            var cb = document.createElement("input");
            cb.type = "checkbox";
            cb.id = name + "_enabled";
            cb.style.margin = "0";
            cb.onchange = function () {
                enabled[name] = this.checked;
                console.log("enabled[" + name + "]=" + enabled[name]);
                sendCommand(name);
            };
            var label = document.createElement("label");
            label.htmlFor = name + "_enabled";
            label.innerHTML = " Enable";
            var div = document.createElement("div");
            div.appendChild(cb);
            div.appendChild(label);
            return div;
        }

        function sendCommand(device) {
            var d = new Date();
            var now = d.getTime();
            if (undefined == commands[device]) commands[device] = 0;
            if (undefined == lastCommands[device]) lastCommands[device] = 0;
            if (undefined == nextCommands[device]) nextCommands[device] = 0;
            if ((now - lastCommands[device]) >= rate) {
                var req = new XMLHttpRequest();
                var url = urlBase + "api/control?device=" + device + "&";
                switch (types[device]) {
                    case "stepper":
                        url += "command=" + (enabled[device] ? commands[device] : 0);
                        break;
                    case "led":
                        url += "enable=" + enabled[device];
                        break;
                    default:
                        console.log("Unknown device type: " + types[device]);
                }
                console.log(url);
                req.open("GET", url, true);
                req.send();
                lastCommands[device] = now;
            }
            else if (nextCommands[device] <= now) {
                console.log("delaying...");
                nextCommands[device] = now + rate;
                setTimeout(sendCommand, rate, device);
            }
        }
    </script>
</body>

</html>

)====";
