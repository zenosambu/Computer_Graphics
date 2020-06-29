function [pos,parent,name,dofs,xforms]=readbvh(fn)

f = fopen(fn,'rt');
parentstack = [];
i = 1;

rotx = @(a) [1 0 0 0; 0 cos(a) -sin(a) 0; 0 sin(a) cos(a) 0; 0 0 0 1]
roty = @(a) [cos(a) 0 sin(a) 0; 0 1 0 0; -sin(a) 0 cos(a) 0; 0 0 0 1]
rotz = @(a) [cos(a) -sin(a) 0 0; sin(a) cos(a) 0 0; 0 0 1 0; 0 0 0 1]
xform = @(vals) [1 0 0 sym(vals{1}); 0 1 0 sym(vals{2}); 0 0 1 sym(vals{3}); 0 0 0 1] * rotz(sym(vals{4})) * roty(sym(vals{5})) * roty(sym(vals{6}))

while ~feof(f),
    s = strsplit(strtrim(fgets(f)));
    if strcmp(s{1},'ROOT') == 1,
        name{i} = s{2};
        parent(i) = 0;
        parentstack(end+1) = 1;
    elseif strcmp(s{1},'JOINT') == 1,
        i = i+1;
        name{i} = s{2};
        parent(i) = parentstack(end);
        parentstack(end+1) = i;
    elseif strcmp(s{1},'End') == 1,
        i = i+1;
        name{i} = strcat(name{parentstack(end)},'_end');
        parent(i) = parentstack(end);
        parentstack(end+1) = i;
        dofs{i} = [];
    elseif strcmp(s{1},'OFFSET') == 1,
        pos(parentstack(end),:) = [ str2double(s{2}), str2double(s{3}), str2double(s{4}) ];
    elseif strcmp(s{1},'}') == 1,
        parentstack = parentstack(1:end-1);
    elseif strcmp(s{1},'CHANNELS') == 1,
        for j=3:length(s),
            varname{j-2} = sprintf('%s_%d',s{j},parentstack(end));
        end
        dofs{parentstack(end)} = varname;
        clear varname;
    end
    if strcmp(s{1},'HIERARCHY') == 0 && isempty(parentstack),
        break;
    end
end

for i=1:size(pos,1),
    xforms{i} = [];
    if i == 1,
        xforms{i} = xform(dofs{i});
    elseif i ~= 1 && ~isempty(dofs{i}),
        xforms{i} = xform(cat(2,num2cell(pos(i,:)), dofs{i}));
        xforms{i} = xforms{parent(i)}*xforms{i};
    end
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
    text( cp(1)+0.5*randn(1), cp(2)+0.5*randn(1), cp(3), strrep(name{i},'_','\_') );
end

for i=1:size(pos,1),
    if parent(i) ~= 0,
    	line( [endpos(i,1), endpos(parent(i),1)], [endpos(i,2), endpos(parent(i),2)], [endpos(i,3), endpos(parent(i),3)] );
    end
end

axis equal