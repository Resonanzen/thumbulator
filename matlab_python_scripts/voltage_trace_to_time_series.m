load 1.txt
X1(:,1) = linspace(0,25.273,25274);
ts = timeseries(X1(:,2),X1(:,1))

save('trace','ts','-v7.3');