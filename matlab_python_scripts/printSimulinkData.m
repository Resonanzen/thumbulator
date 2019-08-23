ts_to_print = v_cap{3}.Values;
size_of_time_series = size(ts_to_print.time);


array_to_print = [transpose(ts_to_print.time); transpose(ts_to_print.Data)];


fileID = fopen('simulink_data.txt','wt');
formatSpec = '%6.3f %12.8f\n';
fprintf(fileID,formatSpec,array_to_print);

fclose(fileID);