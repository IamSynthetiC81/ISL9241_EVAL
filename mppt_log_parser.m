%% Clean

clc; clear all; close all;

%% Parse lines

function lines = parseFile(filename, startString)

  lines = NaN;

  fid = fopen(filename, 'r');
  lines = textscan(fid, '%s', 'Delimiter', '\n');
  fclose(fid);

  lines = lines{1};

 % Find the index of the line that contains the target string
  start_index = NaN;
  for i = 1:length(lines)
      if strcmp(lines{i}, startString)
          start_index = i;
          break;
      end
  end

  % If the target string is not found, do nothing
  if isnan(start_index)
      return;
  end

  lines = lines(start_index:end);
end

function [min_current, max_current, step_size, vi_curve_data] = parse_vi_curve(lines)

  min_current = NaN;
  max_current = NaN;
  step_size = NaN;
  vi_curve_data = NaN;

  startIndex = NaN;
  endIndex = NaN;

  dataIndex = 1;

  for i = 1:length(lines)
    line = lines{i};

    if startsWith(line, 'minimum current')
      min_current = str2double(strsplit(line, ':'){2});
    elseif startsWith(line, 'maximum current')
      max_current = str2double(strsplit(line, ':'){2});
    elseif startsWith(line, 'step size')
      step_size = str2double(strsplit(line, ':'){2});
    elseif index(line, 'SETUP COMPLETE !!') != 0
      endIndex = i-1;
      break;
    elseif strcmp(line, '/#########') || strcmp(line, '/..........\\')
      startIndex = i+1;
    elseif length(strsplit(line, ',')) == 3
      if isnan(startIndex)
        continue;
      end

      strsplitted = strsplit(line, ',');
      strsplitted{1,1} = strrep(strsplitted{1,1}, 'V', '');
      strsplitted{1,2} = strrep(strsplitted{1,2}, 'A', '');
      strsplitted{1,3} = strrep(strsplitted{1,3}, 'A', '');

      vi_curve_data(dataIndex, 1) = str2num(strsplitted{1,1});
      vi_curve_data(dataIndex, 2) = str2num(strsplitted{1,2});
      vi_curve_data(dataIndex, 3) = str2num(strsplitted{1,3});

      dataIndex = dataIndex + 1;
    else
      continue;
    end
  end
end

function [samplingPeriod, Voltage, Current, Power] = parse_continous_data(lines)

  Voltage = NaN;
  Current = NaN;
  Power = NaN;
  samplingPeriod = NaN;

  dataIndex = 1;

  for i = 1:length(lines)
    line = lines{i};

    if startsWith(line, 'Sampling Period')
      samplingPeriod = str2double(strsplit(line, ':'){2})*0.001;

    elseif length(strsplit(line, ',')) == 6
      strsplitted = strsplit(line, ',');
      
      Voltage(dataIndex) = str2num(strsplitted{2});
      Current(dataIndex) = str2num(strsplitted{3});
      Power(dataIndex) = str2num(strsplitted{4});

      dataIndex = dataIndex + 1;
    else
      continue;
    end
  end
end


lines = parseFile('log/log.txt','~=~=~=MPPT Charge Controller=~=~=~');
[min_current, max_current, step_size, vi_curve_data] = parse_vi_curve(lines);

power = vi_curve_data(:,1) .* vi_curve_data(:,2);
max_power = max(power);
max_power_index = find(power == max_power);

printf('Max Power : %2.3fW\n', max_power);
printf('Voltage : %2.3fV\n', vi_curve_data(max_power_index, 1));
printf('Current : %2.3fA\n', vi_curve_data(max_power_index, 2));

figure 1;
ax = plotyy(vi_curve_data(:,1), vi_curve_data(:,2), vi_curve_data(:,1), power);
xlabel('Voltage (V)');
ylabel(ax(1), 'Current (A)');
ylabel(ax(2), 'Power (W)');
title('VI Curve');
grid on;

%% Plot continous data
figure 2;

[samplingPeriod, Voltage, Current, Power] = parse_continous_data(lines);

time = 0:samplingPeriod:(length(Voltage)-1)*samplingPeriod;

subplot(3,1,1);
plot(time, Voltage);
xlabel('Time (s)');
ylabel('Voltage (V)');
title('Voltage vs Time');
grid on;

subplot(3,1,2);
plot(time, Current);
xlabel('Time (s)');
ylabel('Current (A)');
title('Current vs Time');
grid on;

subplot(3,1,3);
plot(time, Power);
xlabel('Time (s)');
ylabel('Power (W)');
title('Power vs Time');
grid on;

