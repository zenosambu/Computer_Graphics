function [pos,parent]=readskel(fn)

f = fopen(fn,'rt');
i = 1;
while ~feof(f),
    s = fgets(f);
    res = regexp(s,'(-*[0-9.]+) (-*[0-9.]+) (-*[0-9.]+) (-*[0-9]+) (.*)','tokens');
    pos(i,:) = [ str2double(res{1}{1}), str2double(res{1}{2}), str2double(res{1}{3}) ];
    parent(i) = str2double(res{1}{4}) + 1;
    name{i} = strrep(res{1}{5},'_','\_');
    i = i+1;
end

clf; hold on
for i=1:size(pos,1),
    cp = pos(i,:);
    j = i;
    while parent(j) ~= 0,
        j = parent(j);
        cp = cp + pos(j,:);
    end
    endpos(i,:) = cp;
    scatter3( cp(1), cp(2), cp(3) );
    text( cp(1)+0.01*randn(1), cp(2)+0.01*randn(1), cp(3), name{i} );
end

for i=1:size(pos,1),
    if parent(i) ~= 0,
    	line( [endpos(i,1), endpos(parent(i),1)], [endpos(i,2), endpos(parent(i),2)], [endpos(i,3), endpos(parent(i),3)] );
    end
end

axis equal