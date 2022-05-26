-- Check Table
create table check_names(
    check_name varchar(32) unique not null,
    check_id int not null AUTO_INCREMENT,
    primary key (check_id)
);
-- Status table
create table check_status(
    ts timestamp NOT NULL,
    check_id int not null,
    comment varchar(96),
    check_status int not null
);

-- Allows only to add a record, if the check has altered state or meaning
DELIMITER //

create trigger check_status_tr
    before insert on check_status
    for each row
begin
    Declare cstatus int;
    Declare ccomment varchar(96);
    select check_status, comment into cstatus, ccomment from check_status where check_id = NEW.check_id order by ts desc LIMIT 1;
    if cstatus = NEW.check_status and NEW.comment = ccomment then
        -- Abort insertion
        SIGNAL SQLSTATE '45000'; 
    end if;
end;

//

DELIMITER ;

-- drop trigger check_status_tr;