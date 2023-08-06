%Author: David Monschein
%License: MIT
%2020-11-15

k = 273.15;
%Masuring results
Rmat = [];
Tmat = [];

Rplotmax = 8000;

% ofset to 2nd row due to title
csvResult = csvread("samples.csv", 1, 0);
[csvHeight,csvWidth] = size(csvResult);
%Load CSV
for i = 1:(csvHeight/2)
   Tmat = [Tmat csvResult(i,1)];
   Rmat = [Rmat csvResult(i,2)];
end
% Kelvin!
Tmat = Tmat + k;
% Get lowest & Highest value from temperature
[T1, It1] = min(Tmat);
[T2, It2] = max(Tmat);
% Get Resistances for them
R1 = Rmat(It1);
R2 = Rmat(It2);
% B
B = T1*T2/(T2-T1)*log(R1/R2)
% use values above for rating
Tr = T2
Rr = R2

%Create Exponential
Rtv = linspace(0,Rplotmax,100);
Rtlog = logspace(1,3,100);
%Rtv
tex = linspace(-20,100,100);
k = 273.15;
% R(T)
%Rntc = Rr * exp(1/(tex + k) + 1/Tr);
Rntc = Rr*exp(B*(1./(tex + k) - 1/Tr))
% T(R)
Tntc = 1./(log(Rtlog ./ Rr)/B + 1/Tr) - k;

Tmat = Tmat - k;

%error("oida")

%Exponential
figure(1)
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
