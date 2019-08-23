
 
wave(:,1) = linspace(0,25.273,25274);
wave(:,2) = 0.1;
ts = timeseries(wave(:,2),wave(:,1))
save('constant_voltage','ts','-v7.3');

