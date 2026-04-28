USE inventory_system;

-- DROP TABLE IF EXISTS devices;
-- DROP TABLE IF EXISTS employees;
-- DROP TABLE IF EXISTS departments;
-- DROP TABLE IF EXISTS creation_log;

-- CREATE TABLE departments (
--     department_id VARCHAR(50) PRIMARY KEY,
--     department_name VARCHAR(100) NOT NULL,
-- 	description VARCHAR(200),
-- 	timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
-- );

-- CREATE TABLE employees (
--     employee_id VARCHAR(50) PRIMARY KEY,
--     name VARCHAR(50) NOT NULL,
--     department_id VARCHAR(50),
--     is_active BOOLEAN DEFAULT TRUE,
-- 	timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,   
--     FOREIGN KEY (department_id) REFERENCES departments(department_id)
-- );

-- CREATE TABLE assets (
-- 	asset_id VARCHAR(50) AUTO_INCREMENT PRIMARY KEY,
--     asset_tag VARCHAR(50) UNIQUE,         -- internal ID
--     serial_number VARCHAR(100),    -- manufacturer SN
--     brand VARCHAR(50),
--     model VARCHAR(100),

--     status VARCHAR(20),
-- 	
-- 	department_id VARCHAR(50), 
--     assigned_employee VARCHAR(50),

--     purchase_date DATE,
--     created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
-- 	note VARCHAR(200),
--     
--     FOREIGN KEY (department_id) REFERENCES departments(department_id),
--     FOREIGN KEY (assigned_employee) REFERENCES employees(employee_id)
-- );

-- CREATE TABLE creation_log (
-- 	id int,
--     asset_index int,
--     asset_year int,
--     asset_tag int
-- );




