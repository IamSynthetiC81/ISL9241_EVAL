file = char("log/putty.txt");


lines = parseFile(file, '~=~=~=MPPT Charge Controller=~=~=~');

[min_current, max_current, step_size, vi_curve_data] = parse_vi_curve(lines);

power = vi_curve_data(:,1) .* vi_curve_data(:,2);
[max_power,max_power_index] = max(power);

printf('Max Power : %2.3fW\n',max_power);
printf('Voltage : %2.3fV\n', vi_curve_data(max_power_index, 1));
printf('Current : %2.3fA\n', vi_curve_data(max_power_index, 2));


figure(1);
subplot(2,1,1);
plot(vi_curve_data(:,1), vi_curve_data(:,2), 'DisplayName',file(5:6));
xlabel('Voltage (V)');
ylabel('Current (A)');
title('VI Curve');
grid on; hold on;
legend();

subplot(2,1,2);
plot(vi_curve_data(:,1), power, 'DisplayName',file(5:6));
xlabel('Voltage (V)');
ylabel('Power (W)');
title('Power vs Voltage');
grid on; hold on;
legend();

figure(2);
[samplingPeriod, Voltage, Current, Power] = parse_continous_data(lines);
t = 0 : samplingPeriod : samplingPeriod*(length(Voltage)-1);

subplot(3,1,1);
plot(t, Voltage, 'DisplayName',file(5:6)); hold on; grid on;
xlabel("Time (s)");
ylabel("Voltage (V)");
legend();

subplot(3,1,2);
plot(t, Current, 'DisplayName',file(5:6)); hold on; grid on;
xlabel("Time (s)");
ylabel("Current (A)");
legend();

subplot(3,1,3);
plot(t, Power, 'DisplayName',file(5:6)); hold on; grid on;
xlabel("Time (s)");
ylabel("Power (W)");
legend();