%Author: David Monschein
%License: MIT
%2020-11-15

%Masuring results
Rmat = [4760  4170  3150  2400  1700  1550  1340  1140  940 750];
Tmat = [22    28    34    40    47    49    55    58    64  69];
RTpolynom = polyfit(Rmat,Tmat,3);
PolyString = polyout(RTpolynom,'Rt')

figure(1)
%Shuow Measuring Results
subplot(2,1,1);
stem(Tmat,Rmat,'linewidth',2,'color','red','filled');
title('Messwerte der Temperaturfühler');
ylabel('R [Ohm]');
xlabel('T [°C]');
grid on;

Rtv = linspace(0,6000,100);
subplot(2,1,2);
plot(Rtv,polyval(RTpolynom,Rtv),'linewidth',2);
title({"Polynomfunktion";PolyString;});

ylabel('T [°C]');
xlabel('R [Ohm]');
hold on
scatter(Rmat,Tmat,'filled');
hold off
grid on
legend('Polynomfunktion','Messwerte');