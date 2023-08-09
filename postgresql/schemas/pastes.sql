CREATE OR REPLACE FUNCTION pseudo_encrypt(value int) returns int AS $$
DECLARE
l1 int;
l2 int;
r1 int;
r2 int;
i int:=0;
BEGIN
 l1:= (value >> 16) & 65535;
 r1:= value & 65535;
 WHILE i < 3 LOOP
   l2 := r1;
   r2 := l1 # ((((1366 * r1 + 150889) % 714025) / 714025.0) * 32767)::int;
   l1 := l2;
   r1 := r2;
   i := i + 1;
 END LOOP;
 return ((r1 << 16) + l1);
END;
$$ LANGUAGE plpgsql strict immutable;

CREATE OR REPLACE FUNCTION stringify_bigint(n bigint) RETURNS text AS $$
DECLARE
 alphabet text:='abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
 base int:=length(alphabet); 
 _n bigint:=abs(n);
 output text:='';
BEGIN
 LOOP
   output := output || substr(alphabet, 1+(_n%base)::int, 1);
   _n := _n / base; 
   EXIT WHEN _n=0;
 END LOOP;
 RETURN output;
END;
$$ LANGUAGE plpgsql strict immutable;

CREATE TABLE IF NOT EXISTS pastes (
    id SERIAL PRIMARY KEY,
    code VARCHAR,
    content VARCHAR,
    token UUID DEFAULT(gen_random_uuid())
);

create or replace function insert_pastes_code() returns trigger as $$
begin
    if NEW.code is null then
        NEW.code := stringify_bigint(pseudo_encrypt(NEW.id));
    end if;
    return new;
end;
$$ language plpgsql;

create trigger trig_insert_pastes_code
before insert
on pastes
for each row
execute procedure insert_pastes_code();