%Author: David Monschein
%License: MIT
%2020-11-15

%Masuring results
Rmat = [];
Tmat = [];

Rplotmax = 40000;

csvResult = csvread("NTC_Messdaten.csv");
[csvHeight,csvWidth] = size(csvResult);
%Load CSV
for i = 1:(csvHeight/2)
   Tmat = [Tmat csvResult(i,1)];
   Rmat = [Rmat csvResult(i,2)];
end
%Create Regression
RTpolynom = polyfit(Rmat,Tmat,5);
PolyString = polyout(RTpolynom,'Rt')
disp(PolyString)

%Create Exponential
Rtv = linspace(0,Rplotmax,100);
Rtlog = logspace(0,5,100);
%Rtv
R25 = 4530;
B = 4048.76;
tex = linspace(-20,100,100);
k = 273.15;
T25 = 25 + k;
%Rntc = R25*(R25.^((1 ./ (tex+273.15))-(1 ./ (25+273.15))));

Rntc = R25 .* exp(B .* ((1 ./ (tex .+ k))-(1/T25)));
%This line Works somehow...
Tntc = (1 ./ ((log(Rtlog ./ R25) ./ B) .+ 1/T25)) .- k;
%Tntc = 25 .* exp((B .* ((1 ./ Rtv) .- (1 / R25)))) .- 16;

figure(1)
%Show Measuring Results
subplot(2,1,1);
stem(Rmat,Tmat,'linewidth',2,'color','red','filled');
title('Messwerte der Temperaturfühler');
ylabel('T [°C]');
xlabel('R [Ohm]');
grid on;

subplot(2,1,2);
plot(Rtv ,polyval(RTpolynom,Rtv),'linewidth',2);
hold on
scatter(Rmat,Tmat,'filled');
hold off
grid on
legend('Polynomfunktion','Messwerte');
ylabel('T [°C]');
xlabel('R [Ohm]');
title({"Polynomfunktion";PolyString;});
%Exponential
figure(2)
subplot(2,1,1);
plot(tex ,Rntc,'linewidth',2,'color','magenta');
hold on
scatter(Tmat,Rmat,'filled');
hold off
grid on
legend('Exponentialfunktion R(T)','Messwerte');
xlabel('T [°C]');
ylabel('R [Ohm]');
title("Exponential B-Formel");
%Reverse Exponential
subplot(2,1,2);
plot(Rtlog ,Tntc,'linewidth',2);
hold on
scatter(Rmat,Tmat,'filled');
hold off
grid on
legend('Umkehrfunktion','Messwerte');
ylabel('T [°C]');
xlabel('R [Ohm]');
title("Logharitmische B-Formel T(R)");
