
clc ; close all;


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

lines = parseFile('log/log.txt', '~=~=~=MPPT Charge Controller=~=~=~');
[min_current,max_current, step_size, vi_curve_data] = parse_vi_curve(lines);

## Write algorithm here

data = unique(vi_curve_data(:,1:2), "rows");
Voltage = data(:,1);
Current = data(:,2);
Power = Voltage .* Current;

figure(1);
subplot(2,1,1);
plot(Voltage, Current); grid on;
xlabel("Voltage (V)");
ylabel("Current (A)");

subplot(2,1,2);
plot(Voltage, Power); grid on;
xlabel("Voltage (V)");
ylabel("Power (W)");

[max_power, max_power_index] = max(Power);

printf("Target Power is %2.3fW at index [%d]\n\n",max_power, max_power_index);

##[v,indx] = max(Voltage);
indx = 5;
v = Voltage(indx);
i = Current(indx);
p = v*i;

printf("%2.3fV, %2.3fA, %2.3fW\n", Voltage(indx), i, p);

t = 1:400;

for ind = 2:400
  v(ind) = Voltage(indx);
  i(ind) = Current(indx);
  p(ind) = i(ind)*v(ind);

  dv = v(ind)-v(ind-1);
  di = i(ind)-i(ind-1);
  dp = p(ind)-p(ind-1);

  printf("dv = [%2.3f] dp = [%2.3f] ", dv, dp);
  printf("Index is %d\n", indx);

  step = 1;

  if mod(ind, 20) == -1
    % Periodic perturbation
    perturb = true;
  else
    perturb = false;
  end

  if perturb
      indx -= 5;
  elseif dp > 0
      if dv > 0
          indx = indx + step;
      elseif dv < 0
          indx = indx - step;
      end
  elseif dp < 0
      if dv > 0
          indx = indx - step;
      elseif dv < 0
          indx = indx + step;
      end
  else
      indx = indx - step;
  end


  if indx > length(Voltage)
    indx = length(Voltage);
  elseif indx < 1
    indx = 1
  end
end

figure(2);

subplot(3,1,1);
stem(t,v); grid on;
xlabel("n Cycles");
ylabel("Voltage (V)");

subplot(3,1,2);
stem(t,i); grid on;
xlabel("n Cycles");
ylabel("Current (A)");

subplot(3,1,3);
stem(t,p); grid on;
xlabel("n Cycles");
ylabel("Power (W)");
