%
%   Blizschlag Juli 2023
%
%   Nach einem Blitzschlag wurde der Temperaturfühler am Dach beschädigt.
%   Der ADC am ESP32 misst noch richtig. Es ist aber noch die übliche
%   Temperaturkennlinie zu sehen nur mit falschen ergebnissen.
%
%   Zur ermittling der neuen Funktion wird von den Falschen messwerten die
%   Temperatur mit dem Solarvorlauf verglichen.
%
k = 273.15;
csvResult = csvread("blitzschlag_Juli23.csv");
[csvHeight,csvWidth] = size(csvResult);

if csvHeight < 2
   error("Zu wenige CSV daten. Es muss minderstens 2 Geben")
end

% declare variables
Tbmat = []; % Blitzschlagfehler Temperatur
Tvmat = []; % Solarvorlauf temperatur
%Load CSV
for i = 1:(csvHeight)
   Tvmat = [Tvmat csvResult(i,1)];
   Tbmat = [Tbmat csvResult(i,2)];
end

% Funktion für den Alten wiederstand
function Rold = Rntc(Told)
  % Alte temperaturformel. Vor dem Blitzschlag
  % Told ist die Matrix zu temperaturen
  k = 273.15;
  R25 = 4530;
  B = 4048.76;
  T25 = 25 + k;

  Rold = R25 .* exp(B .* ((1 ./ (Told + k))-(1/T25)));
endfunction
% Aktuelle Wiederstände nach dem Blitzschlag
Rbmat = Rntc(Tbmat);
Tvmat
Tbmat
Rbmat
% B formel
Btnum = (k + Tvmat(1)) * (k + Tvmat(2));
Btdenum = Tvmat(2) - Tvmat(1);
Bt = Btnum/Btdenum; % temperature part of formula
B = Bt * log(Rbmat(1)/Rbmat(2))
%B = log(Rbmat(1)/Rbmat(2)) / (1/Tbmat(1) - 1/Tbmat(2))

% Temperatur formel
Rtspace = linspace(0,4000,100);
Rr = Rbmat(1)
Tr = Tvmat(1)
Tmat = [];
Tmat = 1./(1/(Tr + k) + log(Rtspace ./ Rr)/B) - k;

plot(Rtspace ,Tmat,'linewidth',2);
hold on
scatter(Rbmat,Tvmat,'filled');
hold off
grid on
legend('Umkehrfunktion','Messwerte');
ylabel('T [°C]');
xlabel('R [Ohm]');
title("Logharitmische B-Formel T(R)");
