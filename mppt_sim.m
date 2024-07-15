%% Clear

clc ; close all;
%% Setup

lines = parseFile('log/log.txt', '~=~=~=MPPT Charge Controller=~=~=~');
[min_current,max_current, step_size, vi_curve_data] = parse_vi_curve(lines);

%% Algorithm Setup

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

fprintf("Target Power is %2.3fW at index [%d]\n\n",max_power, max_power_index);

[v,indx] = max(Voltage);

t = 1:400;

i = 1:400;
p = 1:400;
history = 1:400;

for ind = 1:400
    v(ind) = Voltage(indx);
    i(ind) = Current(indx);
    p(ind) = i(ind)*v(ind);

    history(ind) = indx;

    if ind == 1
        dv = v(ind);
        di = i(ind);
        dp = p(ind);
    else
        dv = v(ind)-v(ind-1);
        di = i(ind)-i(ind-1);
        dp = p(ind)-p(ind-1);
    end

    fprintf("dv = [%2.3f] dp = [%2.3f] ", dv, dp);
    fprintf("Index is %d\n", indx);

    step = 1;

    if mod(ind, 20) == 0
        % Periodic perturbation
        perturb = true;
    else
        perturb = false;
    end

    if perturb
        indx = indx - 5*step;
    elseif dp > 0
        if dv > 0
            indx = indx + 2*step;
        elseif dv < 0
            indx = indx - 2*step;
        else 
            indx = indx + step;
        end
    elseif dp < 0
        if dv > 0
            indx = indx - 2*step;
        elseif dv < 0
            indx = indx + 2*step;
        else
            indx = indx - step;
        end
    else
        if dv >= 0
            indx = indx - step;
        else
            indx = indx + step;
        end
    end


    if indx > length(Voltage)
        indx = length(Voltage);
    elseif indx < 1
        indx = 1;
    end
end

figure(2);

subplot(3,1,1);
plot(t,v); grid on;
xlabel("n Cycles");
ylabel("Voltage (V)");

subplot(3,1,2);
plot(t,i); grid on;
xlabel("n Cycles");
ylabel("Current (A)");

subplot(3,1,3);
plot(t,p); grid on;
xlabel("n Cycles");
ylabel("Power (W)");

%% Heatmap of algorithm

heatmap = 1:length(Power);
for k = 1:length(Power)
    for j=1:length(p)
        if v(j) == Voltage(k) && i(j) == Current(k)
           heatmap(k) = heatmap(k) + 1;
       end
    end
end

heatmap = heatmap/max(heatmap);

figure(3);
plot(Voltage, Power); grid on; hold on;

% Create a colormap based on the probability vector
cmap = colormap(jet(100)); % You can choose other colormaps like 'hot', 'parula', etc.
colors = interp1(linspace(0, 1, size(cmap, 1)), cmap, heatmap);

for k = 1:length(Voltage)-1
    fill([Voltage(k) Voltage(k+1) Voltage(k+1) Voltage(k)], [0 0 Power(k+1) Power(k)], colors(k,:), 'EdgeColor', 'none');
end

colorbar;
clim([0 1]); % Assuming probability values range from 0 to 1
grid on;
%% Functions

function lines = parseFile(filename, startString)
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

  dataIndex = 1;
  
  for i = 1:length(lines)
    line = lines{i};
    if startsWith(line, 'minimum current')
        splitted_line = strsplit(line, ':');
        min_current = str2double(splitted_line(2));
    elseif startsWith(line, 'maximum current')
        splitted_line = strsplit(line, ':');
        max_current = str2double(splitted_line (2));
    elseif startsWith(line, 'step size')
        splitted_line = strsplit(line, ':');
      step_size = str2double(splitted_line(2));
    elseif length(strsplit(line, ',')) == 3
      strsplitted = strsplit(line, ',');
      
      strsplitted{1,1} = strrep(strsplitted{1,1}, 'V', '');
      strsplitted{1,2} = strrep(strsplitted{1,2}, 'A', '');
      strsplitted{1,3} = strrep(strsplitted{1,3}, 'A', '');

      vi_curve_data(dataIndex, 1) = str2double(strsplitted{1,1});
      vi_curve_data(dataIndex, 2) = str2double(strsplitted{1,2});
      vi_curve_data(dataIndex, 3) = str2double(strsplitted{1,3});

      dataIndex = dataIndex + 1;
    else
      continue;
    end
  end
end

