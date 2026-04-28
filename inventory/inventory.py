import mysql.connector
import copy
from nicegui import ui
import pandas as pd

import datetime
import asyncio


def connect_database():
    db = mysql.connector.connect(
        host="localhost", user="admin", password="12345678", database="inventory_system"
    )
    return db


@ui.page("/", title="Inventory Management System")
def main_page():
    MainPage().build()


@ui.page("/departments", title="Departments")
def departments_page():
    DepartmentsPage().build()


@ui.page("/employees", title="Employees")
def employees_page():
    EmployeesPage().build()


@ui.page("/assets", title="Assets")
def assets_page():
    AssetsPage().build()


class UIHelpers:

    @classmethod
    def init(cls):

        cls.success_box = (
            ui.element("div")
            .classes(
                "fixed h-20 bottom-10 right-10 bg-green-500 text-black px-6 py-4 shadow-2xl text-lg"
            )
            .style(
                "opacity: 0; transform: translateY(20px); transition: all 0.3s ease; display: none; z-index: 1000;"
            )
        )

        with cls.success_box:
            ui.label("Transaction successful").classes(
                "w-full h-full flex items-center justify-center text-center font-semibold text-lg"
            )

    @classmethod
    def show_success(cls):

        cls.success_box.style("display: block; opacity: 1;")

        ui.timer(3.0, lambda: cls.success_box.style("opacity: 0"), once=True)
        ui.timer(3.3, lambda: cls.success_box.style("display: none;"), once=True)

    @staticmethod
    def display_error(error_message):

        with ui.dialog() as error_dialog, ui.card().classes(
            "w-40% p-0 overflow-hidden"
        ):

            with ui.column().classes("bg-red-600 text-white w-full px-5 py-4"):
                ui.label("Error").classes("text-xl font-semibold tracking-wide")

            with ui.column().classes("px-5 py-4"):
                ui.label(f"{error_message}").classes(
                    "text-gray-700 font-bold text-xl leading-relaxed"
                )

            with ui.row().classes("justify-end p-4 pt-0"):
                ui.button("Close", on_click=error_dialog.close).classes(
                    "!bg-blue-600 text-white hover:!bg-blue-700"
                )

        error_dialog.open()

    def build_side_bar():

        # |-----------side bar-----------|
        with ui.left_drawer(fixed=True, elevated=True).classes(
            "bg-gray-900 text-white p-0"
        ).props("width=60") as drawer:

            with ui.column().classes("items-center gap-2 p-1 w-full"):

                # HOME
                ui.button(icon="home", on_click=lambda: ui.navigate.to("/")).tooltip(
                    "Home"
                ).props("flat round").classes("w-full hover:bg-gray-700")

                # EMPLOYEES
                ui.button(
                    icon="people", on_click=lambda: ui.navigate.to("/employees")
                ).tooltip("Employees").props("flat round").classes(
                    "w-full hover:bg-gray-700"
                )

                # DEPARTMENTS
                ui.button(
                    icon="apartment", on_click=lambda: ui.navigate.to("/departments")
                ).tooltip("Departments").props("flat round").classes(
                    "w-full hover:bg-gray-700"
                )

                # ASSETS
                ui.button(
                    icon="inventory_2", on_click=lambda: ui.navigate.to("/assets")
                ).tooltip("Assets").props("flat round").classes(
                    "w-full hover:bg-gray-700"
                )


class EmployeesPage:

    def __init__(self):
        self.db = connect_database()

        # main tables
        self.all_rows = []
        self.table = []
        self.onhand = []

        # input fields
        self.fields_add = {}
        self.fields_edit = {}

        # dialog
        self.edit_a_record_dialog = ui.dialog()
        self.add_a_record_dialog = ui.dialog()
        self.onhand_dialog = ui.dialog()
        self.error_dialog = ui.dialog()

        self.edit_enable = []
        self.enabled = True

        self.total_label = []

    def load_departments(self):

        cursor = self.db.cursor()

        query = "SELECT department_id, department_name FROM departments"
        cursor.execute(query)
        departments = {row[0]: row[1] for row in cursor.fetchall()}

        return departments

    def employee_exists(self, value):

        # query employee id from database
        sql = "SELECT employee_id FROM employees WHERE employee_id = %s"

        cursor = self.db.cursor()
        cursor.execute(sql, (value,))
        result = cursor.fetchall()

        # check whether this employee id already exists or not
        # return false if is unique, return true otherwise
        return len(result) > 0

    def reset_add(self):

        # reset all fields
        for field in self.fields_add.values():

            if not field.value == "":
                field.set_value("")
                field.error = False
                field.props(remove="error")

        # close the add a record popup
        self.add_a_record_dialog.close()

    def save_employee(self):

        cursor = self.db.cursor()

        query = (
            "INSERT INTO employees (name, employee_id, department_id) VALUES (%s,%s,%s)"
        )
        val = (
            self.fields_add["name"].value,
            self.fields_add["emp_id"].value,
            self.fields_add["department"].value,
        )

        cursor.execute(query, val)
        self.db.commit()
        # remove previosly keyed in data
        self.reset_add()
        # refresh table
        self.load_data()
        # update count
        self.update_count()
        # display success message
        ui.timer(0.1, UIHelpers.show_success, once=True)

    def save_edit(self):

        # update database
        cursor = self.db.cursor()
        query = f"""UPDATE employees e 
                INNER JOIN departments d ON e.department_id = d.department_id 
                SET e.name = '{self.fields_edit["name"].value}',
                    e.department_id = '{self.fields_edit["department"].value}',
                    e.timestamp = NOW(),
                    e.is_active = '{1 if self.enabled else 0}'
                WHERE e.employee_id = '{self.fields_edit["emp_id"].value}'"""

        cursor.execute(query)
        self.db.commit()

        # refresh
        self.load_data()
        # close dialog
        self.edit_a_record_dialog.close()
        # display success message
        ui.timer(0.1, UIHelpers.show_success, once=True)

    def update_count(self):
        shown = len(self.table.options["rowData"])
        total = len(self.all_rows)
        self.total_label.set_text(f"Showing {shown} of {total} records")

    def fetch_data(self):
        cursor = self.db.cursor()

        cursor.execute(
            """
            SELECT e.employee_id, e.name, d.department_name, e.is_active, e.timestamp
            FROM employees e
            LEFT JOIN departments d 
            ON e.department_id = d.department_id
        """
        )

        return cursor.fetchall()

    def load_data(self):

        rows = self.fetch_data()

        self.all_rows = [
            {
                "id": str(r[0]),
                "name": r[1] or "",
                "dept": r[2] or "",
                "status": "Active" if r[3] else "Inactive",
                "update_date": r[4].strftime("%d-%m-%Y"),
                "update_time": r[4].strftime("%I:%M:%S %p"),
            }
            for r in rows
        ]

        self.table.options["rowData"] = self.all_rows.copy()
        self.table.update()

    def apply_filter(self, value):

        keyword = (value or "").lower()

        if not keyword:
            filtered = self.all_rows
        else:
            filtered = [
                row
                for row in self.all_rows
                if keyword in str(row["id"]).lower()
                or keyword in (row["name"] or "").lower()
                or keyword in (row["dept"] or "").lower()
                or keyword in str(row["update_date"]).lower()
                or keyword in str(row["update_time"]).lower()
            ]

        self.table.options["rowData"] = filtered
        self.table.update()

        # update row count
        self.update_count()

    def switch_state(self):

        self.enabled = not self.enabled

    async def export_data(self, table, file):

        if not isinstance(table, list):

            selection = await table.get_selected_rows()
            if len(selection) == 0:
                UIHelpers().display_error("Please select a record to export")

            else:

                df = pd.DataFrame(selection)
                csv = df.to_csv(index=False).encode("utf-8")

                ui.download(csv, filename=f"{file}.csv")
                UIHelpers().show_success()

    async def validate(self, save_button):

        # if user doesnt key in name and employee id, save button will be disabled
        await asyncio.sleep(0.2)

        if (
            not self.fields_add["name"].value
            or not self.fields_add["emp_id"].value
            or not self.fields_add["department"].value
        ):
            save_button.disable()

        # dont allow for duplicate id
        elif self.employee_exists(self.fields_add["emp_id"].value):
            save_button.disable()

        else:
            save_button.enable()

    async def validate_edit(self, save_button):

        # if user doesnt key in name and employee id, save button will be disabled
        await asyncio.sleep(0.2)

        if (
            not self.fields_edit["name"].value
            or not self.fields_edit["emp_id"].value
            or not self.fields_edit["department"].value
        ):
            save_button.disable()

        else:
            save_button.enable()

    async def delete_record(self):

        cursor = self.db.cursor()
        query = "DELETE FROM employees WHERE employee_id = (%s)"

        selected = await self.table.get_selected_rows()
        for row in selected:
            val = (row["id"],)

            cursor.execute(query, val)
            self.db.commit()

        # refresh
        self.load_data()
        self.update_count()
        # show success message
        UIHelpers.show_success()

    async def open_edit_dialog(self):

        selected = await self.table.get_selected_rows()
        if len(selected) != 1:
            UIHelpers.display_error(
                "Multiple records selected. Please select only one record to proceed with editing."
            )

        elif len(selected) == 0:
            UIHelpers.display_error(
                "No records selected. Please select only a record to proceed with editing."
            )

        else:
            self.edit_record(selected)

    def edit_record(self, selected):

        cursor = self.db.cursor()
        query = """
            SELECT e.employee_id, e.name, d.department_id, e.timestamp, e.is_active
            FROM employees e
            LEFT JOIN departments d 
            ON e.department_id = d.department_id
            WHERE e.employee_id = (%s)
        """

        val = (selected[0]["id"],)

        cursor.execute(query, val)
        results = cursor.fetchall()[0]

        # load in values for each field
        # name
        self.fields_edit["name"].value = results[1]
        # employee id
        self.fields_edit["emp_id"].value = results[0]

        # switch the state of enable mode
        self.edit_enable.on_change = None  # disable handler

        self.edit_enable.set_value(True if results[4] == 1 else False)
        self.enabled = True if results[4] == 1 else False
        self.edit_enable.on_change = self.switch_state  # restore handler

        self.edit_enable.update()

        # disable this field
        self.fields_edit["emp_id"].disable()

        # department (department used as key)
        self.fields_edit["department"].value = results[2]
        # time stamp
        self.fields_edit["timestamp"].set_text(
            results[3].strftime("%d-%m-%Y %I:%M:%S %p")
        )

        # update the fields before opening
        for field in self.fields_edit.values():
            field.update()

        self.edit_a_record_dialog.open()

    def open_add_dialog(self):

        # update the fields before opening
        for field in self.fields_add.values():
            field.update()

        self.add_a_record_dialog.open()

    def open_onhand_dialog(self, rowInfo):

        emp_id = rowInfo.args["data"]["id"]

        cursor = self.db.cursor()
        query = f"SELECT asset_tag, serial_number, brand, model, status, issued_date FROM assets WHERE assigned_employee = '{emp_id}'"
        cursor.execute(query)
        result = cursor.fetchall()

        self.onhand.options["rowData"] = [
            {
                "asset_tag": r[0] or "",
                "serial_number": r[1] or "",
                "brand": r[2] or "",
                "model": r[3],
                "status": r[4],
                "issued_date": r[5],
            }
            for r in result
        ]

        self.onhand.update()
        self.onhand_dialog.open()

    def build_onhand_record(self):

        # a popup that shows all the peripherals / assets that were borrowed by this user
        # add a employee popup form
        with self.onhand_dialog, ui.card().classes(
            "w-3/4 h-full p-6 overflow-auto"
        ).style("max-width: none"):
            # header row
            with ui.row().classes("w-full justify-between items-center"):
                # Title
                ui.label("Assets Held").classes("text-2xl font-bold")

                #  Cancel button
                ui.button(
                    icon="close", on_click=lambda: self.onhand_dialog.close()
                ).props("flat round").classes("!text-gray-700")

            with ui.card().classes("w-full h-9/10"):

                self.onhand = ui.aggrid(
                    {
                        "columnDefs": [
                            {
                                "headerName": "",
                                "field": "select",
                                "checkboxSelection": True,
                                "headerCheckboxSelection": True,
                                "width": 50,
                                "pinned": "left",
                                "lockPosition": True,
                                "suppressMovable": True,
                                "resizable": False,
                                "cellClass": "center-checkbox",
                                "headerClass": "center-header",
                            },
                            {
                                "headerName": "Asset Tag",
                                "field": "asset_tag",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Serial number",
                                "field": "serial_number",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Brand",
                                "field": "brand",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Model",
                                "field": "model",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Status",
                                "field": "status",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Issued date",
                                "field": "issued_date",
                                "filter": True,
                                "sortable": True,
                            },
                        ],
                        "rowData": [],
                        "rowSelection": "multiple",
                        "suppressMovableColumns": False,  # keep others movable
                        "suppressDragLeaveHidesColumns": True,
                    }
                ).classes("h-full")

                # cancel / save
                with ui.row().classes("w-full items-center justify-end gap-2 mt-4"):

                    # export a record
                    with ui.button(
                        on_click=lambda: self.export_data(self.onhand, "Asset OnHand")
                    ).props("flat").classes(""):
                        ui.icon("upload").classes("p-2 text-gray-700")
                        ui.label("Export Data").classes("text-gray-700")

                    # cancel button
                    ui.button(
                        "Cancel", on_click=lambda: self.onhand_dialog.close()
                    ).props("flat").classes("!text-gray-700")

    def build_add_record(self):

        # |-----------add a record-----------|
        # add a employee popup form
        with self.add_a_record_dialog, ui.card().classes("w-120 p-6 overflow-hidden"):
            # header row
            with ui.row().classes("w-full justify-between items-center"):
                # Title
                ui.label("Add Employee").classes("text-2xl font-bold")

                #  Cancel button
                ui.button(icon="close", on_click=self.add_a_record_dialog.close).props(
                    "flat round"
                ).classes("!text-gray-700")

            # input fields
            with ui.column().classes("w-full gap-8") as input_fields:
                # employee name input
                self.fields_add["name"] = (
                    ui.input(
                        "Name",
                        validation={
                            "Required field": lambda value: True if value else None
                        },
                    )
                    .classes("w-1/2")
                    .props('outlined hide-bottom-space clearable autocomplete="off"')
                )

                #  employee id input
                self.fields_add["emp_id"] = (
                    ui.input(
                        "Employee ID",
                        validation={
                            "Duplicate ID": lambda value: not self.employee_exists(
                                value
                            ),
                            "Required field": lambda value: True if value else None,
                        },
                    )
                    .classes("w-1/2")
                    .props('outlined hide-bottom-space clearable autocomplete="off"')
                )

                # dropdown for select department
                self.fields_add["department"] = (
                    ui.select(
                        label="Department",
                        with_input=True,
                        options=self.load_departments(),
                        validation={
                            "Required field": lambda value: True if value else None
                        },
                    )
                    .classes("w-1/2")
                    .props("outlined")
                )

            # cancel / save
            with ui.row().classes("w-full justify-end gap-2 mt-4"):

                # cancel button
                ui.button("Cancel", on_click=lambda: self.reset_add()).props(
                    "flat"
                ).classes("!text-gray-700")
                # save button
                save_button = ui.button(
                    "Save", on_click=lambda: self.save_employee()
                ).classes("!bg-blue-600 text-white hover:!bg-blue-700")

            # validate on popup load
            asyncio.create_task(self.validate(save_button))
            # validate name and employee id
            self.fields_add["name"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate(save_button)),
            )
            self.fields_add["emp_id"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate(save_button)),
            )
            self.fields_add["department"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate(save_button)),
            )

    def build_edit_record(self):

        # |-----------edit a record-----------|
        # edit an employee popup form
        with self.edit_a_record_dialog, ui.card().classes("w-120 p-6 overflow-auto"):
            # header row
            with ui.row().classes("w-full justify-between items-center"):
                # Title
                ui.label("Edit Employee").classes("text-2xl font-bold")

                #  Cancel button
                ui.button(icon="close", on_click=self.edit_a_record_dialog.close).props(
                    "flat round"
                ).classes("!text-gray-700")

            # input fields
            with ui.column().classes("w-full gap-6 p-4") as input_fields:
                # employee name input
                self.fields_edit["name"] = (
                    ui.input(
                        "Name",
                        validation={
                            "Required field": lambda value: True if value else None
                        },
                    )
                    .classes("w-1/2")
                    .props('outlined hide-bottom-space clearable autocomplete="off"')
                )

                #  employee id input
                self.fields_edit["emp_id"] = (
                    ui.input("Employee ID")
                    .classes("w-1/2")
                    .props('outlined hide-bottom-space clearable autocomplete="off"')
                )

                # dropdown for select department
                self.fields_edit["department"] = (
                    ui.select(
                        label="Department",
                        with_input=True,
                        options=self.load_departments(),
                        validation={
                            "Required field": lambda value: True if value else None
                        },
                    )
                    .classes("w-1/2")
                    .props("outlined")
                )

            # toggle switch
            self.edit_enable = ui.switch(
                "Enable", on_change=lambda: self.switch_state()
            )

            # separator
            ui.separator().style("background-color: gray; height: 1px;")

            # timestamp
            with ui.column().classes("w-full gap-8 p-4"):

                with ui.row():
                    ui.label("Last Updated :").classes(
                        "text-base font-semibold text-gray-800"
                    )

                    self.fields_edit["timestamp"] = ui.label("").classes(
                        "text-base text-gray-400 font-medium"
                    )

            # cancel / save
            with ui.row().classes("w-full justify-end gap-2 mt-4"):

                # cancel button
                ui.button("Cancel", on_click=self.edit_a_record_dialog.close).props(
                    "flat"
                ).classes("!text-gray-700")
                # save button
                save_button = ui.button(
                    "Save", on_click=lambda: self.save_edit()
                ).classes("!bg-blue-600 text-white hover:!bg-blue-700")

            # validate on popup load
            asyncio.create_task(self.validate_edit(save_button))

            # validate name and employee id
            self.fields_edit["name"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate_edit(save_button)),
            )
            self.fields_edit["department"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate_edit(save_button)),
            )
            self.edit_enable.on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate_edit(save_button)),
            )

    def build_main_page(self):

        # |-----------side bar-----------|
        UIHelpers.build_side_bar()

        # |-----------main page-----------|
        with ui.column().classes("w-full h-screen p-4"):

            ui.label("Employee List").classes("text-xl font-bold")

            with ui.row().classes("w-full items-center gap-2"):

                # search bar
                search = (
                    ui.input(label="Search", placeholder="Type to filter...")
                    .classes("w-1/3")
                    .props('outlined autocomplete="off"')
                )

                # add a record button
                with ui.button(on_click=lambda: self.open_add_dialog()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("add_box").classes("p-2 text-gray-700")
                    ui.label("Add A Record").classes("text-gray-700")

                # edit a record button
                with ui.button(on_click=lambda: self.open_edit_dialog()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("edit").classes("p-2 text-gray-700")
                    ui.label("Edit A Record").classes("text-gray-700")

                # delete a record button
                with ui.button(on_click=lambda: self.delete_record()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("delete").classes("p-2 text-gray-700")
                    ui.label("Delete A Record").classes("text-gray-700")

                # export a record
                with ui.button(
                    on_click=lambda: self.export_data(self.table, "Employee List")
                ).props("flat").classes("flex items-center justify-center"):
                    ui.icon("upload").classes("p-2 text-gray-700")
                    ui.label("Export Data").classes("text-gray-700")

                # refresh button
                with ui.button(on_click=lambda: self.load_data()).props(
                    "flat round dense"
                ).tooltip("Refresh"):
                    ui.icon("refresh").classes("text-gray-700")

            # outer border
            with ui.card().classes("w-full").style("height: 75vh"):

                # with ui.element("div").classes("scroll-container"):

                # |-----------employee table-----------|
                self.table = ui.aggrid(
                    {
                        "columnDefs": [
                            {
                                "headerName": "",
                                "field": "select",
                                "checkboxSelection": True,
                                "headerCheckboxSelection": True,
                                "width": 50,
                                "pinned": "left",
                                "lockPosition": True,
                                "suppressMovable": True,
                                "resizable": False,
                                "cellClass": "center-checkbox",
                                "headerClass": "center-header",
                            },
                            {
                                "headerName": "Name",
                                "field": "name",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Employee ID",
                                "field": "id",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Department",
                                "field": "dept",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Status",
                                "field": "status",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Update Date",
                                "field": "update_date",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Update Time",
                                "field": "update_time",
                                "filter": True,
                                "sortable": True,
                            },
                        ],
                        "rowData": [],
                        "rowSelection": "multiple",
                        "suppressMovableColumns": False,  # keep others movable
                        "suppressDragLeaveHidesColumns": True,
                        "suppressRowClickSelection": True,
                    }
                ).classes("h-full")

                self.table.on(
                    "cellDoubleClicked", lambda value: self.open_onhand_dialog(value)
                )
                self.total_label = ui.label("Showing 0 of 0 records")

            # load for first time
            self.load_data()
            # show count for first time
            self.update_count()
            # apply filter
            search.on_value_change(lambda e: self.apply_filter(e.value))

    def build(self):

        ui.add_head_html(
            """
            <style>
            /* center header text */
            .center-header .ag-header-cell-label {
                justify-content: center !important;
            }

            /* center checkbox perfectly */
            .center-checkbox {
                display: flex !important;
                justify-content: center !important;
                align-items: center !important;
            }

            /* fix AG Grid checkbox container */
            .ag-selection-checkbox {
                width: 100%;
                display: flex !important;
                justify-content: center !important;
            }
            </style>
        """
        )
        UIHelpers().init()
        # |-----------show onhand items for employee-----------|
        self.build_onhand_record()
        # |-----------add a record-----------|
        self.build_add_record()
        # |-----------edit a record-----------|
        self.build_edit_record()
        # |-----------main page-----------|
        self.build_main_page()


class DepartmentsPage:

    def __init__(self):
        self.db = connect_database()

        # main tables
        self.all_rows = []
        self.table = []

        # input fields
        self.fields_add = {}
        self.fields_edit = {}

        # dialog
        self.edit_a_record_dialog = ui.dialog()
        self.add_a_record_dialog = ui.dialog()
        self.error_dialog = ui.dialog()

        self.total_label = []

    async def export_data(self):

        # convert AG grid table into pd dataframe to be exported using ui.download
        if not isinstance(self.table, list):

            selection = await self.table.get_selected_rows()
            if len(selection) == 0:
                UIHelpers().display_error("Please select a record to export")

            else:

                df = pd.DataFrame(selection)
                csv = df.to_csv(index=False).encode("utf-8")

                ui.download(csv, filename="Assets List.csv")
                UIHelpers().show_success()

    def reset_add(self):

        # reset all fields
        for field in self.fields_add.values():

            if not field.value == "":
                field.set_value("")
                field.error = False
                field.props(remove="error")

        # close the add a record popup
        self.add_a_record_dialog.close()

    def save_department(self):

        cursor = self.db.cursor()

        query = "INSERT INTO departments (department_name, department_id, description) VALUES (%s,%s,%s)"
        val = (
            self.fields_add["name"].value,
            self.fields_add["id"].value,
            self.fields_add["description"].value,
        )

        cursor.execute(query, val)
        self.db.commit()
        # remove previosly keyed in data
        self.reset_add()
        # refresh table
        self.load_data()
        # update count
        self.update_count()
        # display success message
        ui.timer(0.1, UIHelpers.show_success, once=True)

    def load_departments(self):

        cursor = self.db.cursor()

        query = "SELECT department_id, department_name FROM departments"
        cursor.execute(query)
        departments = {row[0]: row[1] for row in cursor.fetchall()}

        return departments

    def department_exists(self, value):

        # query employee id from database
        sql = "SELECT department_id FROM departments WHERE department_id = %s"

        cursor = self.db.cursor()
        cursor.execute(sql, (value,))
        result = cursor.fetchall()

        # check whether this department id already exists or not
        # return false if is unique, return true otherwise
        return len(result) > 0

    def save_edit(self):

        # update database
        cursor = self.db.cursor()
        query = f"""UPDATE departments  
                SET department_name = '{self.fields_edit["name"].value}',
                    department_id = '{self.fields_edit["id"].value}',
                    description = '{self.fields_edit["description"].value}',
                    timestamp = NOW()
                WHERE department_id = '{self.fields_edit["id"].value}'"""

        cursor.execute(query)
        self.db.commit()

        # refresh
        self.load_data()
        # close dialog
        self.edit_a_record_dialog.close()
        # display success message
        ui.timer(0.1, UIHelpers.show_success, once=True)

    def update_count(self):

        shown = len(self.table.options["rowData"])
        total = len(self.all_rows)
        self.total_label.set_text(f"Showing {shown} of {total} records")

    def fetch_data(self):
        cursor = self.db.cursor()

        cursor.execute(
            "SELECT department_id, department_name, description, timestamp FROM departments"
        )

        return cursor.fetchall()

    def load_data(self):

        rows = self.fetch_data()

        self.all_rows = [
            {
                "id": str(r[0]),
                "name": r[1] or "",
                "description": r[2] or "",
                "update_date": r[3].strftime("%d-%m-%Y"),
                "update_time": r[3].strftime("%I:%M:%S %p"),
            }
            for r in rows
        ]

        self.table.options["rowData"] = self.all_rows.copy()
        self.table.update()

    def apply_filter(self, value):

        keyword = (value or "").lower()

        if not keyword:
            filtered = self.all_rows
        else:
            filtered = [
                row
                for row in self.all_rows
                if keyword in str(row["id"]).lower()
                or keyword in (row["name"] or "").lower()
                or keyword in (row["description"] or "").lower()
                or keyword in str(row["update_date"]).lower()
                or keyword in str(row["update_time"]).lower()
            ]

        self.table.options["rowData"] = filtered
        self.table.update()

        # update row count
        self.update_count()

    async def validate(self, save_button):

        # if user doesnt key in name and employee id, save button will be disabled
        await asyncio.sleep(0.2)

        if not self.fields_add["name"].value or not self.fields_add["id"].value:
            save_button.disable()

        # dont allow for duplicate id
        elif self.department_exists(self.fields_add["id"].value):
            save_button.disable()

        else:
            save_button.enable()

    async def validate_edit(self, save_button):

        # if user doesnt key in name and employee id, save button will be disabled
        await asyncio.sleep(0.2)

        if not self.fields_edit["name"].value or not self.fields_edit["id"].value:
            save_button.disable()

        else:
            save_button.enable()

    async def delete_record(self):

        cursor = self.db.cursor()
        # check whether are there any employees in this department before deleting
        query = "DELETE FROM departments WHERE department_id = (%s)"

        selected = await self.table.get_selected_rows()
        for row in selected:
            val = (row["id"],)

            check = "SELECT COUNT(*) FROM employees WHERE department_id = (%s)"
            cursor.execute(check, val)

            if cursor.fetchall()[0][0] == 0:
                cursor.execute(query, val)
                self.db.commit()

            else:
                UIHelpers().display_error(
                    f"Department {val[0]} can't be deleted as it is still assigned to employees"
                )

        # refresh
        self.load_data()
        self.update_count()
        # show success message
        UIHelpers().show_success()

    async def open_edit_dialog(self):

        selected = await self.table.get_selected_rows()
        if len(selected) != 1:
            UIHelpers.display_error(
                "Multiple records selected. Please select only one record to proceed with editing."
            )

        elif len(selected) == 0:
            UIHelpers.display_error(
                "No records selected. Please select only a record to proceed with editing."
            )

        else:
            self.edit_record(selected)

    def edit_record(self, selected):

        cursor = self.db.cursor()
        query = f"""
            SELECT department_name, department_id, description, timestamp
            FROM departments
            WHERE department_id = '{selected[0]["id"]}'
        """

        cursor.execute(query)
        results = cursor.fetchall()[0]

        # load in values for each field
        # name
        self.fields_edit["name"].value = results[0]
        # department id
        self.fields_edit["id"].value = results[1]

        # disable this field
        self.fields_edit["id"].disable()

        # description
        self.fields_edit["description"].value = results[2]
        # time stamp
        self.fields_edit["timestamp"].set_text(
            results[3].strftime("%d-%m-%Y %I:%M:%S %p")
        )

        # update the fields before opening
        for field in self.fields_edit.values():
            field.update()

        self.edit_a_record_dialog.open()

    def open_add_dialog(self):

        # update the fields before opening
        for field in self.fields_add.values():
            field.update()

        self.add_a_record_dialog.open()

    def build_add_record(self):

        # |-----------add a record-----------|
        # add a department popup form
        with self.add_a_record_dialog, ui.card().classes("w-120 p-6 overflow-y: auto"):
            # header row
            with ui.row().classes("w-full justify-between items-center"):
                # Title
                ui.label("Add Department").classes("text-2xl font-bold")

                #  Cancel button
                ui.button(icon="close", on_click=self.add_a_record_dialog.close).props(
                    "flat round"
                ).classes("!text-gray-700")

            # input fields
            with ui.column().classes("w-full gap-8") as input_fields:

                with ui.row().classes("w-full justify-between"):

                    # department name input
                    self.fields_add["name"] = (
                        ui.input(
                            "Name",
                            validation={
                                "Required field": lambda value: True if value else None
                            },
                        )
                        .classes("w-9/20")
                        .props(
                            'outlined hide-bottom-space clearable autocomplete="off"'
                        )
                    )

                    #  department id input
                    self.fields_add["id"] = (
                        ui.input(
                            "Department ID",
                            validation={
                                "Required field": lambda value: True if value else None,
                            },
                        )
                        .classes("w-9/20")
                        .props(
                            'outlined hide-bottom-space clearable autocomplete="off"'
                        )
                    )

                # description
                self.fields_add["description"] = (
                    ui.textarea(label="Description")
                    .classes("w-full")
                    .props("outlined counter maxlength=200")
                )

            # cancel / save
            with ui.row().classes("w-full justify-end gap-2 mt-4"):

                # cancel button
                ui.button("Cancel", on_click=lambda: self.reset_add()).props(
                    "flat"
                ).classes("!text-gray-700")
                # save button
                save_button = ui.button(
                    "Save", on_click=lambda: self.save_department()
                ).classes("!bg-blue-600 text-white hover:!bg-blue-700")

            # validate on popup load
            asyncio.create_task(self.validate(save_button))
            # validate name and employee id
            self.fields_add["name"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate(save_button)),
            )
            self.fields_add["id"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate(save_button)),
            )

    def build_edit_record(self):

        # |-----------edit a record-----------|
        # edit an department popup form
        with self.edit_a_record_dialog, ui.card().classes(
            "w-1/2 p-6 overflow-y: auto;"
        ):
            # header row
            with ui.row().classes("w-full justify-between items-center"):
                # Title
                ui.label("Edit Department").classes("text-2xl font-bold")

                #  Cancel button
                ui.button(icon="close", on_click=self.edit_a_record_dialog.close).props(
                    "flat round"
                ).classes("!text-gray-700")

            # input fields
            with ui.column().classes("w-full gap-8 p-4") as input_fields:

                with ui.row().classes("w-full justify-between"):
                    # department name input
                    self.fields_edit["name"] = (
                        ui.input(
                            "Name",
                            validation={
                                "Required field": lambda value: True if value else None
                            },
                        )
                        .classes("w-9/20")
                        .props(
                            'outlined hide-bottom-space clearable autocomplete="off"'
                        )
                    )

                    #  department id input
                    self.fields_edit["id"] = (
                        ui.input("Department ID")
                        .classes("w-9/20")
                        .props(
                            'outlined hide-bottom-space clearable autocomplete="off"'
                        )
                    )

                # optional description
                self.fields_edit["description"] = (
                    ui.textarea(label="Description")
                    .classes("w-full")
                    .props("outlined counter maxlength=200")
                )

            # separator
            ui.separator().style("background-color: gray; height: 1px;")

            # timestamp
            with ui.column().classes("w-full gap-8 p-4"):

                with ui.row():
                    ui.label("Last Updated :").classes(
                        "text-base font-semibold text-gray-800"
                    )

                    self.fields_edit["timestamp"] = ui.label("").classes(
                        "text-base text-gray-400 font-medium"
                    )

            # cancel / save
            with ui.row().classes("w-full justify-end gap-2 mt-4"):

                # cancel button
                ui.button("Cancel", on_click=self.edit_a_record_dialog.close).props(
                    "flat"
                ).classes("!text-gray-700")
                # save button
                save_button = ui.button(
                    "Save", on_click=lambda: self.save_edit()
                ).classes("!bg-blue-600 text-white hover:!bg-blue-700")

            # validate on popup load
            asyncio.create_task(self.validate_edit(save_button))

            # validate name and employee id
            self.fields_edit["name"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate_edit(save_button)),
            )
            self.fields_edit["description"].on(
                "update:model-value",
                lambda e: asyncio.create_task(self.validate_edit(save_button)),
            )

    def build_main_page(self):

        # |-----------side bar-----------|
        UIHelpers.build_side_bar()

        # |-----------main page-----------|
        with ui.column().classes("w-full h-screen p-4"):

            ui.label("Departments List").classes("text-xl font-bold")

            with ui.row().classes("w-full items-center gap-2"):

                # search bar
                search = (
                    ui.input(label="Search", placeholder="Type to filter...")
                    .classes("w-1/3")
                    .props('outlined autocomplete="off"')
                )

                # add a record button
                with ui.button(on_click=lambda: self.open_add_dialog()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("add_box").classes("p-2 text-gray-700")
                    ui.label("Add A Record").classes("text-gray-700")

                # edit a record button
                with ui.button(on_click=lambda: self.open_edit_dialog()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("edit").classes("p-2 text-gray-700")
                    ui.label("Edit A Record").classes("text-gray-700")

                # delete a record button
                with ui.button(on_click=lambda: self.delete_record()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("delete").classes("p-2 text-gray-700")
                    ui.label("Delete A Record").classes("text-gray-700")

                # export a record
                with ui.button(on_click=self.export_data).props("flat").classes(
                    "flex items-center justify-center"
                ):
                    ui.icon("upload").classes("p-2 text-gray-700")
                    ui.label("Export Data").classes("text-gray-700")

                # refresh button
                with ui.button(on_click=lambda: self.load_data()).props(
                    "flat round dense"
                ).tooltip("Refresh"):
                    ui.icon("refresh").classes("text-gray-700")

            # outer border
            with ui.card().classes("w-full").style("height: 75vh"):

                # with ui.element("div").classes("scroll-container"):

                # |-----------department table-----------|
                self.table = ui.aggrid(
                    {
                        "columnDefs": [
                            {
                                "headerName": "",
                                "field": "select",
                                "checkboxSelection": True,
                                "headerCheckboxSelection": True,
                                "width": 50,
                                "pinned": "left",
                                "lockPosition": True,
                                "suppressMovable": True,
                                "resizable": False,
                                "cellClass": "center-checkbox",
                                "headerClass": "center-header",
                            },
                            {
                                "headerName": "Name",
                                "field": "name",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Department ID",
                                "field": "id",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Description",
                                "field": "description",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Update Date",
                                "field": "update_date",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Update Time",
                                "field": "update_time",
                                "filter": True,
                                "sortable": True,
                            },
                        ],
                        "rowData": [],
                        "rowSelection": "multiple",
                        "suppressMovableColumns": False,  # keep others movable
                        "suppressDragLeaveHidesColumns": True,
                    }
                ).classes("h-full")

                self.total_label = ui.label("Showing 0 of 0 records")

            # load for first time
            self.load_data()
            # show count for first time
            self.update_count()
            # apply filter
            search.on_value_change(lambda e: self.apply_filter(e.value))

    def build(self):

        ui.add_head_html(
            """
            <style>
            /* center header text */
            .center-header .ag-header-cell-label {
                justify-content: center !important;
            }

            /* center checkbox perfectly */
            .center-checkbox {
                display: flex !important;
                justify-content: center !important;
                align-items: center !important;
            }

            /* fix AG Grid checkbox container */
            .ag-selection-checkbox {
                width: 100%;
                display: flex !important;
                justify-content: center !important;
            }
            </style>
        """
        )

        UIHelpers().init()

        # |-----------add a record-----------|
        self.build_add_record()
        # |-----------edit a record-----------|
        self.build_edit_record()
        # |-----------main page-----------|
        self.build_main_page()


class AssetsPage:

    def __init__(self):
        self.db = connect_database()

        # main tables
        self.all_rows = []
        self.table = []
        self.history = []

        # input fields
        self.fields_add = {}
        self.fields_edit = {}

        # dialog
        self.edit_a_record_dialog = ui.dialog()
        self.add_a_record_dialog = ui.dialog()
        self.error_dialog = ui.dialog()
        self.history_dialog = ui.dialog()

        self.total_label = []

    def load_departments(self):

        cursor = self.db.cursor()

        query = "SELECT department_id, department_name FROM departments"
        cursor.execute(query)
        departments = {row[0]: row[1] for row in cursor.fetchall()}

        return departments

    def employee_exists(self, value):

        # query employee id from database
        sql = "SELECT employee_id FROM employees WHERE employee_id = %s"

        cursor = self.db.cursor()
        cursor.execute(sql, (value,))
        result = cursor.fetchall()

        # check whether this employee id already exists or not
        # return false if is unique, return true otherwise
        return len(result) > 0

    def reset_add(self):

        # reset all fields
        for field in self.fields_add.values():

            if isinstance(field, str):
                field = ""

            else:
                if not field.value == "":

                    field.set_value("")
                    field.error = False
                    field.props(remove="error")

        # close the add a record popup
        self.add_a_record_dialog.close()

    def save_asset(self):

        cursor = self.db.cursor()

        query = f"""INSERT INTO assets
        (asset_id, asset_tag, serial_number, brand, model, status, department_id, assigned_employee, issued_date)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
        """

        val = (
            self.fields_add["asset_id"].value,
            self.fields_add["asset_tag"].value,
            self.fields_add["serial_number"].value,
            self.fields_add["brand"].value,
            self.fields_add["model"].value,
            self.fields_add["status"].value,
            self.fields_add["department_id"],
            self.fields_add["employee_id"].value,
            self.fields_add["date"].value if self.fields_add["date"].value else None,
        )

        cursor.execute(query, val)
        self.db.commit()

        # update history log
        self.save_history(self.fields_add)

        # remove previosly keyed in data
        self.reset_add()

        # increment by one
        query = f"""UPDATE creation_log SET 
        asset_index = asset_index + 1, 
        asset_tag = asset_tag + 1,
        asset_year = {datetime.datetime.now().year} WHERE id=0"""

        cursor.execute(query)
        self.db.commit()

        # refresh table
        self.load_data()
        # update count
        self.update_count()
        # display success message
        ui.timer(0.1, UIHelpers.show_success, once=True)

    def save_edit(self):

        # update database
        cursor = self.db.cursor()

        query = f"""UPDATE assets
        SET 
        serial_number = %s, 
        brand = %s,
        model = %s, 
        status = %s, 
        department_id = %s, 
        assigned_employee = %s, 
        issued_date = %s
        WHERE asset_id = %s
        """

        val = (
            self.fields_edit["serial_number"].value,
            self.fields_edit["brand"].value,
            self.fields_edit["model"].value,
            self.fields_edit["status"].value,
            self.fields_edit["department_id"],
            self.fields_edit["employee_id"].value,
            self.fields_edit["date"].value,
            self.fields_edit["asset_id"].value,
        )
        cursor.execute(query, val)
        self.db.commit()

        # update history log
        self.save_history(self.fields_edit)

        # refresh
        self.load_data()
        # close dialog
        self.edit_a_record_dialog.close()
        # display success message
        ui.timer(0.1, UIHelpers.show_success, once=True)

    def save_history(self, field):

        cursor = self.db.cursor()

        # update history log
        query = f"""INSERT INTO asset_history
        (asset_id, status, assigned_employee, issued_date, note)
        VALUES (%s, %s, %s, %s, %s)
        """

        val = (
            field["asset_id"].value,
            field["status"].value,
            field["employee_id"].value,
            (
                field["date"].value
                if field["date"].value
                else datetime.date.today().strftime("%Y-%m-%d")
            ),
            field["note"].value,
        )

        cursor.execute(query, val)
        self.db.commit()

    def update_count(self):
        shown = len(self.table.options["rowData"])
        total = len(self.all_rows)
        self.total_label.set_text(f"Showing {shown} of {total} records")

    def fetch_data(self):

        cursor = self.db.cursor()

        cursor.execute(
            """
            SELECT a.asset_id, a.asset_tag, a.serial_number, a.brand, a.model, a.status, e.name, d.department_name, a.issued_date, a.timestamp
            FROM (assets a
            LEFT JOIN departments d 
            ON a.department_id = d.department_id) LEFT JOIN
            employees e
            ON a.assigned_employee = e.employee_id
        """
        )

        return cursor.fetchall()

    def load_data(self):

        rows = self.fetch_data()

        self.all_rows = [
            {
                "id": str(r[0]),
                "tag": r[1] or "",
                "serial_number": r[2] or "",
                "brand": r[3] or "",
                "model": r[4] or "",
                "status": r[5] or "",
                "employee": r[6] or "",
                "dept": r[7] or "",
                "issued_date": (r[8] if r[8] else ""),
                "update_timestamp": (
                    r[9].strftime("%d-%m-%Y %I:%M:%S %p") if r[9] else ""
                ),
            }
            for r in rows
        ]

        self.table.options["rowData"] = self.all_rows.copy()
        self.table.update()

    def apply_filter(self, value):

        keyword = (value or "").lower()

        if not keyword:
            filtered = self.all_rows
        else:
            filtered = [
                row
                for row in self.all_rows
                if keyword in str(row["id"]).lower()
                or keyword in (row["tag"] or "").lower()
                or keyword in (row["serial_number"] or "").lower()
                or keyword in (row["brand"] or "").lower()
                or keyword in (row["model"] or "").lower()
                or keyword in (row["status"] or "").lower()
                or keyword in (row["employee"] or "").lower()
                or keyword in (row["dept"] or "").lower()
                or keyword in str(row["issued_date"]).lower()
                or keyword in str(row["update_timestamp"]).lower()
            ]

        self.table.options["rowData"] = filtered
        self.table.update()

        # update row count
        self.update_count()

    async def export_data(self, table, file):

        if not isinstance(table, list):

            selection = await table.get_selected_rows()
            if len(selection) == 0:
                UIHelpers().display_error("Please select a record to export")

            else:

                df = pd.DataFrame(selection)
                csv = df.to_csv(index=False).encode("utf-8")

                ui.download(csv, filename=f"{file}.csv")
                UIHelpers().show_success()

    async def validate(self, field, save_button):

        # if user doesnt key in name and employee id, save button will be disabled
        await asyncio.sleep(0.2)

        save_button.enable()
        if (
            not field["status"].value
            or not field["serial_number"].value
            or not field["brand"].value
            or not field["model"].value
            or not (
                (
                    field["status"].value in ["Active", "Repair"]
                    and field["employee_id"].value
                )
                or (
                    field["status"].value not in ["Active", "Repair"]
                    and not field["employee_id"].value
                )
            )
        ):
            save_button.disable()

        if not (
            (
                field["status"].value in ["Active", "Repair"]
                and self.employee_exists(field["employee_id"].value)
            )
            or (
                field["status"].value not in ["Active", "Repair"]
                and (not field["employee_id"].value)
            )
        ):
            save_button.disable()
            # disable button when no employee name and status is active repair

    async def delete_record(self):

        cursor = self.db.cursor()
        query = "DELETE FROM assets WHERE asset_id = (%s)"

        selected = await self.table.get_selected_rows()
        for row in selected:
            val = (row["id"],)

            cursor.execute(query, val)
            self.db.commit()

        # refresh
        self.load_data()
        self.update_count()
        # show success message
        UIHelpers.show_success()

    async def disable_fields(self, field, save_button):

        # disable the employee id input field when status of item not active or in repair
        if not (field["status"].value in ["Active", "Repair"]):

            field["employee_id"].value = ""
            field["employee_name"].value = ""
            field["department"].value = ""
            field["department_id"] = ""

            field["employee_id"].disable()

            field["employee_id"].update()
            field["employee_name"].update()
            field["department"].update()

        else:
            field["employee_id"].enable()
            field["employee_id"].update()

        field["employee_id"].validate()

        await self.validate(field, save_button)

    async def open_edit_dialog(self):

        selected = await self.table.get_selected_rows()
        if len(selected) != 1:
            UIHelpers.display_error(
                "Multiple records selected. Please select only one record to proceed with editing."
            )

        elif len(selected) == 0:
            UIHelpers.display_error(
                "No records selected. Please select only a record to proceed with editing."
            )

        else:
            self.edit_record(selected)

    def open_history_dialog(self, rowInfo):

        asset_id = rowInfo.args["data"]["id"]

        cursor = self.db.cursor()
        query = f"""SELECT ah.issued_date, ah.assigned_employee, e.name, ah.status, ah.note
            FROM asset_history ah LEFT JOIN employees e
            ON ah.assigned_employee = e.employee_id
            WHERE ah.asset_id = (%s)
            ORDER BY ah.issued_date DESC
            """

        cursor.execute(query, (asset_id,))
        result = cursor.fetchall()

        self.history.options["rowData"] = [
            {
                "issued_date": r[0] or "",
                "employee_id": r[1] or "",
                "employee_name": r[2] or "",
                "status": r[3] or "",
                "note": r[4],
            }
            for r in result
        ]

        self.history.update()
        self.history_dialog.open()

    def load_employee(self, field, value):

        # if employee is not valid, return false
        # if employee is valid, update the employee name and department

        if self.employee_exists(value):

            # query employee id from database
            sql = f"SELECT e.name, d.department_name, d.department_id FROM employees e INNER JOIN departments d on e.department_id = d.department_id WHERE e.employee_id = '{value}'"

            cursor = self.db.cursor()
            cursor.execute(sql)
            result = cursor.fetchall()[0]

            # assign values
            field["department_id"] = result[2]
            field["employee_name"].value = result[0]
            field["department"].value = result[1]

            # update the fields
            field["employee_name"].update()
            field["department"].update()

            return True

        else:

            # assign values
            field["department_id"] = ""
            field["employee_name"].value = ""
            field["department"].value = ""

            # update the fields
            field["employee_name"].update()
            field["department"].update()

            return False

    def edit_record(self, selected):

        cursor = self.db.cursor()
        query = f"""
            SELECT asset_id, asset_tag, serial_number, brand, model, status, department_id, assigned_employee, issued_date, timestamp
            FROM assets
            WHERE asset_tag = %s
        """
        cursor.execute(query, (selected[0]["tag"],))
        results = cursor.fetchall()[0]

        # load in values for each field
        # asset id
        self.fields_edit["asset_id"].value = results[0]
        # asset tag
        self.fields_edit["asset_tag"].value = results[1]
        # serial number
        self.fields_edit["serial_number"].value = results[2]
        # brand
        self.fields_edit["brand"].value = results[3]
        # model
        self.fields_edit["model"].value = results[4]
        # status
        self.fields_edit["status"].value = results[5]
        # employee id
        self.fields_edit["employee_id"].value = results[7]
        # issued date
        self.fields_edit["date"].value = results[8]
        # note
        self.fields_edit["note"].value = ""

        # time stamp
        self.fields_edit["timestamp"].set_text(
            results[9].strftime("%d-%m-%Y %I:%M:%S %p")
        )

        # employee department
        query = f"""    
            SELECT department_name FROM departments WHERE department_id = '{results[6]}'
        """
        cursor.execute(query)
        dept = cursor.fetchall()

        if len(dept) == 0:
            self.fields_edit["department"].value = ""
        else:
            self.fields_edit["department"].value = dept[0]

        # employee name
        query = f"""
            SELECT name FROM employees WHERE employee_id = '{results[7]}'
        """
        cursor.execute(query)

        name = cursor.fetchall()
        if len(name) == 0:
            self.fields_edit["employee_name"].value = ""
        else:

            self.fields_edit["employee_name"].value = name[0]

        # update the fields before opening
        for field in self.fields_edit.values():

            if not isinstance(field, str):
                field.update()

        self.edit_a_record_dialog.open()

    def open_add_dialog(self):

        # asset id and asset tag are considered running numbers, thus system will auto generate
        cursor = self.db.cursor()
        query = (
            "SELECT asset_index, asset_year, asset_tag FROM creation_log WHERE id = 0"
        )

        cursor.execute(query)
        data = cursor.fetchall()[0]

        self.fields_add["asset_id"].value = f"{data[0]}"
        self.fields_add["asset_tag"].value = f"DJN-{data[1]}{str(data[2]).zfill(6)}"

        # update the fields before opening
        for field in self.fields_add.values():

            if isinstance(field, ui.input):
                field.update()

        self.add_a_record_dialog.open()

    def build_history(self):

        # dialog that shows all the transaction history of a selected asset
        with self.history_dialog, ui.card().classes(
            "w-3/4 h-full p-6 overflow-auto"
        ).style("max-width: none; max-height:none"):
            # header row
            with ui.row().classes("w-full justify-between items-center"):
                # Title
                ui.label("History").classes("text-2xl font-bold")

                #  Cancel button
                ui.button(
                    icon="close", on_click=lambda: self.history_dialog.close()
                ).props("flat round").classes("!text-gray-700")

            with ui.card().classes("w-full h-9/10"):

                self.history = ui.aggrid(
                    {
                        "columnDefs": [
                            {
                                "headerName": "",
                                "field": "select",
                                "checkboxSelection": True,
                                "headerCheckboxSelection": True,
                                "width": 50,
                                "pinned": "left",
                                "lockPosition": True,
                                "suppressMovable": True,
                                "resizable": False,
                                "cellClass": "center-checkbox",
                                "headerClass": "center-header",
                            },
                            {
                                "headerName": "Issued Date",
                                "field": "issued_date",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Employee ID",
                                "field": "employee_id",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Employee Name",
                                "field": "employee_name",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Status",
                                "field": "status",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Note",
                                "field": "note",
                                "filter": True,
                                "sortable": True,
                            },
                        ],
                        "rowData": [],
                        "rowSelection": "multiple",
                        "suppressMovableColumns": False,  # keep others movable
                        "suppressDragLeaveHidesColumns": True,
                    }
                ).classes("h-full")

                # export record / cancel
                with ui.row().classes("w-full items-center justify-end gap-2 mt-4"):

                    # export a record
                    with ui.button(
                        on_click=lambda: self.export_data(
                            self.history, "Transaction History"
                        )
                    ).props("flat").classes(""):
                        ui.icon("upload").classes("p-2 text-gray-700")
                        ui.label("Export Data").classes("text-gray-700")

                    # cancel button
                    ui.button(
                        "Cancel", on_click=lambda: self.history_dialog.close()
                    ).props("flat").classes("!text-gray-700")

    def build_add_record(self):

        # |-----------add a record-----------|
        # add a asset popup form
        with self.add_a_record_dialog, ui.card().classes(
            "w-150 h-full p-6 overflow-hidden"
        ).style("max-height: none; max-width: none"):

            with ui.tabs().classes("w-full") as tabs:
                tab_general = ui.tab("General")
                tab_note = ui.tab("Note")

            with ui.tab_panels(tabs, value=tab_general).classes("w-full h-full"):

                with ui.tab_panel(tab_general):

                    # header row
                    with ui.row().classes("w-full justify-between items-center"):
                        # Title
                        ui.label("Add Asset").classes("text-2xl font-bold")

                        #  Cancel button
                        ui.button(
                            icon="close", on_click=self.add_a_record_dialog.close
                        ).props("flat round").classes("!text-gray-700")

                    # input fields
                    with ui.column().classes("w-full h-full gap-8") as input_fields:

                        # Row 1
                        with ui.row().classes("w-full justify-between"):
                            # asset id input (disabled)
                            self.fields_add["asset_id"] = (
                                ui.input("Asset ID")
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            #  asset tag input (disabled)
                            self.fields_add["asset_tag"] = (
                                ui.input("Asset Tag")
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )
                            self.fields_add["asset_id"].disable()
                            self.fields_add["asset_tag"].disable()

                        # Row 2
                        with ui.row().classes("w-full justify-between"):
                            # serial number input
                            self.fields_add["serial_number"] = (
                                ui.input(
                                    "Serial Number",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            #  status drop down select
                            self.fields_add["status"] = (
                                ui.select(
                                    options=[
                                        "Active",
                                        "Repair",
                                        "In Inventory",
                                        "Decommissioned",
                                        "Missing",
                                        "Scrapped",
                                    ],
                                    label="Status",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                        # Row 3
                        with ui.row().classes("w-full justify-between"):
                            # brand input
                            self.fields_add["brand"] = (
                                ui.input(
                                    "Brand Name",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            # model input
                            self.fields_add["model"] = (
                                ui.input(
                                    "Model",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                        # Row 4
                        with ui.row().classes("w-full justify-between"):

                            # employee ID input
                            self.fields_add["employee_id"] = (
                                ui.input(
                                    "Employee ID",
                                    validation={
                                        "Required field": lambda value: (
                                            True
                                            if (
                                                self.fields_add["status"].value
                                                in ["Active", "Repair"]
                                                and value
                                            )
                                            or (
                                                self.fields_add["status"].value
                                                not in ["Active", "Repair"]
                                                and not value
                                            )
                                            else None
                                        ),
                                        "Employee does not exist": lambda value: (
                                            True
                                            if (
                                                (
                                                    self.fields_add["status"].value
                                                    in ["Active", "Repair"]
                                                    and self.load_employee(
                                                        self.fields_add, value
                                                    )
                                                )
                                                or (
                                                    self.fields_add["status"].value
                                                    not in ["Active", "Repair"]
                                                    and (not value)
                                                )
                                            )
                                            else None
                                        ),
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            # department input (disabled - auto generated when employee is keyed in)
                            self.fields_add["employee_name"] = (
                                ui.input(
                                    "Employee Name",
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_add["employee_name"].disable()

                        # row 5
                        with ui.row().classes("w-full justify-between"):
                            # department input (disabled - auto generated when employee is keyed in)
                            self.fields_add["department"] = (
                                ui.input(
                                    "Department",
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_add["date"] = (
                                ui.date_input("Purchase Date")
                                .classes("w-9/20")
                                .props("outlined hide-bottom-space clearable")
                            )

                            self.fields_add["department"].disable()

                with ui.tab_panel(tab_note):

                    self.fields_add["note"] = (
                        ui.textarea("Note")
                        .classes("w-full h-3/5")
                        .props(
                            'outlined hide-bottom-space clearable maxlength=200 autocomplete="off"'
                        )
                    )

            # cancel / save
            with ui.row().classes("w-full justify-end gap-2 mt-4"):

                # cancel button
                ui.button("Cancel", on_click=lambda: self.reset_add()).props(
                    "flat"
                ).classes("!text-gray-700")
                # save button
                save_button = ui.button(
                    "Save", on_click=lambda: self.save_asset()
                ).classes("!bg-blue-600 text-white hover:!bg-blue-700")

            # validate on popup load
            asyncio.create_task(self.validate(self.fields_add, save_button))
            # validate all the input fields
            self.fields_add["serial_number"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_add, save_button)
                ),
            )
            self.fields_add["status"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.disable_fields(self.fields_add, save_button)
                ),
            )
            self.fields_add["brand"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_add, save_button)
                ),
            )
            self.fields_add["model"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_add, save_button)
                ),
            )
            self.fields_add["employee_id"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_add, save_button)
                ),
            )

    def build_edit_record(self):

        # |-----------edit a record-----------|
        # edit an asset popup form
        with self.edit_a_record_dialog, ui.card().classes(
            "w-150 h-full p-6 overflow-hidden"
        ).style("max-height: none; max-width: none"):

            with ui.tabs().classes("w-full") as tabs:
                tab_general = ui.tab("General")
                tab_note = ui.tab("Note")

            with ui.tab_panels(tabs, value=tab_general).classes("w-full h-full"):

                with ui.tab_panel(tab_general):

                    # header row
                    with ui.row().classes("w-full justify-between items-center"):
                        # Title
                        ui.label("Edit Employee").classes("text-2xl font-bold")

                        #  Cancel button
                        ui.button(
                            icon="close", on_click=self.edit_a_record_dialog.close
                        ).props("flat round").classes("!text-gray-700")

                    # input fields
                    with ui.column().classes("w-full gap-8") as input_fields:

                        # Row 1
                        with ui.row().classes("w-full justify-between"):
                            # asset id input (disabled)
                            self.fields_edit["asset_id"] = (
                                ui.input("Asset ID")
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            #  asset tag input (disabled)
                            self.fields_edit["asset_tag"] = (
                                ui.input("Asset Tag")
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )
                            self.fields_edit["asset_id"].disable()
                            self.fields_edit["asset_tag"].disable()

                        # Row 2
                        with ui.row().classes("w-full justify-between"):
                            # serial number input
                            self.fields_edit["serial_number"] = (
                                ui.input(
                                    "Serial Number",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            #  status drop down select
                            self.fields_edit["status"] = (
                                ui.select(
                                    options=[
                                        "Active",
                                        "Repair",
                                        "In Inventory",
                                        "Decommissioned",
                                        "Missing",
                                        "Scrapped",
                                    ],
                                    label="Status",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                        # Row 3
                        with ui.row().classes("w-full justify-between"):
                            # brand input
                            self.fields_edit["brand"] = (
                                ui.input(
                                    "Brand Name",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            # model input
                            self.fields_edit["model"] = (
                                ui.input(
                                    "Model",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                        # Row 4
                        with ui.row().classes("w-full justify-between"):

                            # employee ID input
                            self.fields_edit["employee_id"] = (
                                ui.input(
                                    "Employee ID",
                                    validation={
                                        "Required field": lambda value: (
                                            True
                                            if (
                                                self.fields_edit["status"].value
                                                in ["Active", "Repair"]
                                                and value
                                            )
                                            or (
                                                self.fields_edit["status"].value
                                                not in ["Active", "Repair"]
                                                and not value
                                            )
                                            else None
                                        ),
                                        "Employee does not exist": lambda value: (
                                            True
                                            if (
                                                (
                                                    self.fields_edit["status"].value
                                                    in ["Active", "Repair"]
                                                    and self.load_employee(
                                                        self.fields_edit, value
                                                    )
                                                )
                                                or (
                                                    self.fields_edit["status"].value
                                                    not in ["Active", "Repair"]
                                                    and (not value)
                                                )
                                            )
                                            else None
                                        ),
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            # department input (disabled - auto generated when employee is keyed in)
                            self.fields_edit["employee_name"] = (
                                ui.input(
                                    "Employee Name",
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_edit["employee_name"].disable()

                        # row 5
                        with ui.row().classes("w-full justify-between"):
                            # department input (disabled - auto generated when employee is keyed in)
                            self.fields_edit["department"] = (
                                ui.input(
                                    "Department",
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_edit["date"] = (
                                ui.date_input("Issued Date")
                                .classes("w-9/20")
                                .props(
                                    "outlined hide-bottom-space clearable autocomplete='off'"
                                )
                            )

                            self.fields_edit["department"].disable()

                with ui.tab_panel(tab_note):

                    self.fields_edit["note"] = (
                        ui.textarea("Note")
                        .classes("w-full h-3/5")
                        .props(
                            'outlined hide-bottom-space clearable maxlength=200 autocomplete="off"'
                        )
                    )

            # separator
            ui.separator().style("background-color: gray; height: 1px;")

            # timestamp
            with ui.column().classes("w-full gap-8 p-4"):

                with ui.row():
                    ui.label("Last Updated :").classes(
                        "text-base font-semibold text-gray-800"
                    )

                    self.fields_edit["timestamp"] = ui.label("").classes(
                        "text-base text-gray-400 font-medium"
                    )

            # cancel / save
            with ui.row().classes("w-full justify-end gap-2 mt-4"):

                # cancel button
                ui.button("Cancel", on_click=self.edit_a_record_dialog.close).props(
                    "flat"
                ).classes("!text-gray-700")
                # save button
                save_button = ui.button(
                    "Save", on_click=lambda: self.save_edit()
                ).classes("!bg-blue-600 text-white hover:!bg-blue-700")

            # validate on popup load
            asyncio.create_task(self.validate(self.fields_edit, save_button))

            # validate name and employee id
            # validate all the input fields
            self.fields_edit["serial_number"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )
            self.fields_edit["status"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.disable_fields(self.fields_edit, save_button)
                ),
            )

            self.fields_edit["brand"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )
            self.fields_edit["model"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )
            self.fields_edit["employee_id"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )
            self.fields_edit["date"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )

    def build_main_page(self):

        # |-----------side bar-----------|
        UIHelpers.build_side_bar()
        # |-----------main page-----------|
        with ui.column().classes("w-full h-screen p-4"):

            ui.label("Assets List").classes("text-xl font-bold")

            with ui.row().classes("w-full items-center gap-2"):

                # search bar
                search = (
                    ui.input(label="Search", placeholder="Type to filter...")
                    .classes("w-1/3")
                    .props('outlined autocomplete="off"')
                )

                # add a record button
                with ui.button(on_click=lambda: self.open_add_dialog()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("add_box").classes("p-2 text-gray-700")
                    ui.label("Add A Record").classes("text-gray-700")

                # edit a record button
                with ui.button(on_click=lambda: self.open_edit_dialog()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("edit").classes("p-2 text-gray-700")
                    ui.label("Edit A Record").classes("text-gray-700")

                # delete a record button
                with ui.button(on_click=lambda: self.delete_record()).props(
                    "flat"
                ).classes("flex items-center justify-center"):
                    ui.icon("delete").classes("p-2 text-gray-700")
                    ui.label("Delete A Record").classes("text-gray-700")

                # export a record
                with ui.button(on_click=self.export_data).props("flat").classes(
                    "flex items-center justify-center"
                ):
                    ui.icon("upload").classes("p-2 text-gray-700")
                    ui.label("Export Data").classes("text-gray-700")

                # refresh button
                with ui.button(on_click=lambda: self.load_data()).props(
                    "flat round dense"
                ).tooltip("Refresh"):
                    ui.icon("refresh").classes("text-gray-700")

            # outer border
            with ui.card().classes("w-full").style("height: 75vh"):

                # |-----------asset table-----------|
                self.table = ui.aggrid(
                    {
                        "columnDefs": [
                            {
                                "headerName": "",
                                "field": "select",
                                "checkboxSelection": True,
                                "headerCheckboxSelection": True,
                                "width": 50,
                                "pinned": "left",
                                "lockPosition": True,
                                "suppressMovable": True,
                                "resizable": False,
                                "cellClass": "center-checkbox",
                                "headerClass": "center-header",
                            },
                            {
                                "headerName": "Asset Tag",
                                "field": "tag",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Serial Number",
                                "field": "serial_number",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Brand",
                                "field": "brand",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Model",
                                "field": "model",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Status",
                                "field": "status",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Employee",
                                "field": "employee",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Department",
                                "field": "dept",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Issued Date",
                                "field": "issued_date",
                                "filter": True,
                                "sortable": True,
                            },
                            {
                                "headerName": "Update Timestamp",
                                "field": "update_timestamp",
                                "filter": True,
                                "sortable": True,
                            },
                        ],
                        "rowData": [],
                        "rowSelection": "multiple",
                        "suppressMovableColumns": False,  # keep others movable
                        "suppressDragLeaveHidesColumns": True,
                        "suppressRowClickSelection": True,
                    }
                ).classes("h-full")

                self.table.on(
                    "cellDoubleClicked", lambda value: self.open_history_dialog(value)
                )

                self.total_label = ui.label("Showing 0 of 0 records")

            # load for first time
            self.load_data()
            # show count for first time
            self.update_count()
            # apply filter
            search.on_value_change(lambda e: self.apply_filter(e.value))

    def build(self):

        ui.add_head_html(
            """
            <style>
            /* center header text */
            .center-header .ag-header-cell-label {
                justify-content: center !important;
            }

            /* center checkbox perfectly */
            .center-checkbox {
                display: flex !important;
                justify-content: center !important;
                align-items: center !important;
            }

            /* fix AG Grid checkbox container */
            .ag-selection-checkbox {
                width: 100%;
                display: flex !important;
                justify-content: center !important;
            }
            </style>
        """
        )

        # initialise UIhelpers
        UIHelpers.init()
        # |-----------asset history-----------|
        self.build_history()
        # |-----------add a record-----------|
        self.build_add_record()
        # |-----------edit a record-----------|
        self.build_edit_record()
        # |-----------main page-----------|
        self.build_main_page()


class MainPage:

    def __init__(self):

        self.db = connect_database()
        self.barcode = []

        self.register_dialog = ui.dialog()
        self.query_dialog = ui.dialog()

        self.fields_add = {}
        self.fields_edit = {}

    def load_employee(self, field, value):

        # if employee is not valid, return false
        # if employee is valid, update the employee name and department

        if self.employee_exists(value):

            # query employee id from database
            sql = f"SELECT e.name, d.department_name, d.department_id FROM employees e INNER JOIN departments d on e.department_id = d.department_id WHERE e.employee_id = '{value}'"

            cursor = self.db.cursor()
            cursor.execute(sql)
            result = cursor.fetchall()[0]

            # assign values
            field["department_id"] = result[2]
            field["employee_name"].value = result[0]
            field["department"].value = result[1]

            # update the fields
            field["employee_name"].update()
            field["department"].update()

            return True

        else:

            # assign values
            field["department_id"] = ""
            field["employee_name"].value = ""
            field["department"].value = ""

            # update the fields
            field["employee_name"].update()
            field["department"].update()

            return False

    def save_asset(self):

        cursor = self.db.cursor()

        query = f"""INSERT INTO assets
        (asset_id, asset_tag, serial_number, brand, model, status, department_id, assigned_employee, issued_date)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
        """

        val = (
            self.fields_add["asset_id"].value,
            self.fields_add["asset_tag"].value,
            self.fields_add["serial_number"].value,
            self.fields_add["brand"].value,
            self.fields_add["model"].value,
            self.fields_add["status"].value,
            self.fields_add["department_id"],
            self.fields_add["employee_id"].value,
            self.fields_add["date"].value if self.fields_add["date"].value else None,
        )

        cursor.execute(query, val)
        self.db.commit()

        # update history log
        self.save_history(self.fields_add)

        # remove previosly keyed in data
        self.reset_add()

        # increment by one
        query = f"""UPDATE creation_log SET 
        asset_index = asset_index + 1, 
        asset_tag = asset_tag + 1,
        asset_year = {datetime.datetime.now().year} WHERE id=0"""

        cursor.execute(query)
        self.db.commit()

        # display success message
        ui.timer(0.1, UIHelpers.show_success, once=True)

    def save_edit(self):

        # update database
        cursor = self.db.cursor()

        query = f"""UPDATE assets
        SET 
        serial_number = %s, 
        brand = %s,
        model = %s, 
        status = %s, 
        department_id = %s, 
        assigned_employee = %s, 
        issued_date = %s
        WHERE asset_id = %s
        """

        val = (
            self.fields_edit["serial_number"].value,
            self.fields_edit["brand"].value,
            self.fields_edit["model"].value,
            self.fields_edit["status"].value,
            self.fields_edit["department_id"],
            self.fields_edit["employee_id"].value,
            self.fields_edit["date"].value,
            self.fields_edit["asset_id"].value,
        )
        cursor.execute(query, val)
        self.db.commit()

        # update history log
        self.save_history(self.fields_edit)

        # close dialog
        self.query_dialog.close()
        # display success message
        ui.timer(0.1, UIHelpers.show_success, once=True)

    def save_history(self, field):

        cursor = self.db.cursor()

        # update history log
        query = f"""INSERT INTO asset_history
        (asset_id, status, assigned_employee, issued_date, note)
        VALUES (%s, %s, %s, %s, %s)
        """

        val = (
            field["asset_id"].value,
            field["status"].value,
            field["employee_id"].value,
            (
                field["date"].value
                if field["date"].value
                else datetime.date.today().strftime("%Y-%m-%d")
            ),
            field["note"].value,
        )

        cursor.execute(query, val)
        self.db.commit()

    def load_barcode(self, barcode_name, value):

        ui.run_javascript(
            f"""
            JsBarcode("#{barcode_name}", "{value or 'UNKNOWN'}", {{
                format: "CODE128A",
                width: 2,
                height: 35,
                displayValue: true,
                fontSize: 15,
            }});
            """
        )

    def asset_tag_exists(self, value):

        cursor = self.db.cursor()
        query = "SELECT asset_id, serial_number, brand, model, status, department_id, assigned_employee, issued_date, timestamp FROM assets WHERE asset_tag = %s"

        cursor.execute(query, (value,))
        data = cursor.fetchall()

        if len(data) == 0:
            for key in self.fields_edit:

                field = self.fields_edit[key]
                if not isinstance(field, str):

                    if key != "asset_tag" and not isinstance(field, ui.label):
                        field.value = ""
                        field.disable()
                        field.error = None
                        field.update()

            self.load_barcode("barcode_query", "")
            self.fields_edit["timestamp"].set_text("")

            return False

        else:
            results = data[0]

            # load in values for each field
            # asset id
            self.fields_edit["asset_id"].value = results[0]
            # asset tag
            self.fields_edit["asset_tag"].value = value

            # load barcode
            self.load_barcode("barcode_query", self.fields_edit["asset_tag"].value)

            # serial number
            self.fields_edit["serial_number"].value = results[1]
            self.fields_edit["serial_number"].enable()
            # brand
            self.fields_edit["brand"].value = results[2]
            self.fields_edit["brand"].enable()
            # model
            self.fields_edit["model"].value = results[3]
            self.fields_edit["model"].enable()
            # status
            self.fields_edit["status"].value = results[4]
            self.fields_edit["status"].enable()
            # employee id
            self.fields_edit["employee_id"].value = results[6]
            self.fields_edit["employee_id"].enable()
            # issued date
            self.fields_edit["date"].value = results[7]
            self.fields_edit["date"].enable()
            # note
            self.fields_edit["note"].value = ""

            # time stamp
            self.fields_edit["timestamp"].set_text(
                results[8].strftime("%d-%m-%Y %I:%M:%S %p")
            )

            # employee department
            query = f"""    
                SELECT department_name FROM departments WHERE department_id = '{results[5]}'
            """
            cursor.execute(query)
            dept = cursor.fetchall()

            if len(dept) == 0:
                self.fields_edit["department"].value = ""
            else:
                self.fields_edit["department"].value = dept[0]

            # employee name
            query = f"""
                SELECT name FROM employees WHERE employee_id = '{results[6]}'
            """
            cursor.execute(query)

            name = cursor.fetchall()
            if len(name) == 0:
                self.fields_edit["employee_name"].value = ""
            else:

                self.fields_edit["employee_name"].value = name[0]

            # update the fields once all data has loaded in
            for field in self.fields_edit.values():

                if not isinstance(field, str):
                    field.update()

            return True

    def employee_exists(self, value):

        # query employee id from database
        sql = "SELECT employee_id FROM employees WHERE employee_id = %s"

        cursor = self.db.cursor()
        cursor.execute(sql, (value,))
        result = cursor.fetchall()

        # check whether this employee id already exists or not
        # return false if is unique, return true otherwise
        return len(result) > 0

    def reset_add(self):

        # reset all fields
        for field in self.fields_add.values():

            if isinstance(field, str):
                field = ""

            else:
                if not field.value == "":

                    field.set_value("")
                    field.error = False
                    field.props(remove="error")

        # close the add a record popup
        self.register_dialog.close()

    async def disable_fields(self, field, save_button):

        # disable the employee id input field when status of item not active or in repair
        if not (field["status"].value in ["Active", "Repair"]):

            field["employee_id"].value = ""
            field["employee_name"].value = ""
            field["department"].value = ""
            field["department_id"] = ""

            field["employee_id"].disable()

            field["employee_id"].update()
            field["employee_name"].update()
            field["department"].update()

        else:
            field["employee_id"].enable()
            field["employee_id"].update()

        field["employee_id"].validate()

        await self.validate(field, save_button)

    async def validate(self, field, save_button):

        # if user doesnt key in name and employee id, save button will be disabled
        await asyncio.sleep(0.2)

        save_button.enable()
        if (
            not field["status"].value
            or not field["serial_number"].value
            or not field["brand"].value
            or not field["model"].value
            or not (
                (
                    field["status"].value in ["Active", "Repair"]
                    and field["employee_id"].value
                )
                or (
                    field["status"].value not in ["Active", "Repair"]
                    and not field["employee_id"].value
                )
            )
        ):
            save_button.disable()

        if not (
            (
                field["status"].value in ["Active", "Repair"]
                and self.employee_exists(field["employee_id"].value)
            )
            or (
                field["status"].value not in ["Active", "Repair"]
                and (not field["employee_id"].value)
            )
        ):
            save_button.disable()
            # disable button when no employee name and status is active repair

    def open_register_dialog(self):

        # asset id and asset tag are considered running numbers, thus system will auto generate
        cursor = self.db.cursor()
        query = (
            "SELECT asset_index, asset_year, asset_tag FROM creation_log WHERE id = 0"
        )

        cursor.execute(query)
        data = cursor.fetchall()[0]

        self.fields_add["asset_id"].value = f"{data[0]}"
        self.fields_add["asset_tag"].value = f"DJN-{data[1]}{str(data[2]).zfill(6)}"

        ui.run_javascript(
            f"""
            JsBarcode("#barcode_register", "{self.fields_add["asset_tag"].value or 'EMPTY'}", {{
                format: "CODE128",
                width: 2,
                height: 40,
                displayValue: true,
                fontSize: 15,
            }});
        """
        )

        # update the fields before opening
        for field in self.fields_add.values():

            if isinstance(field, ui.input):
                field.update()

        self.register_dialog.open()

    def open_query_dialog(self):

        self.load_barcode("barcode_query", "")
        self.query_dialog.open()

    def build_register(self):

        # |-----------add a record-----------|
        # add a asset popup form
        with self.register_dialog, ui.card().classes(
            "w-3/5 h-full p-6 overflow-auto"
        ).style("max-width: none; max-height:none"):

            with ui.tabs().classes("w-full") as tabs:
                tab_general = ui.tab("General")
                tab_note = ui.tab("Note")

            with ui.tab_panels(tabs, value=tab_general).classes("w-full h-full"):

                with ui.tab_panel(tab_general).classes("w-full"):

                    # header row
                    with ui.row().classes("w-full justify-between items-center"):
                        # Title
                        ui.label("Add Asset").classes("text-2xl font-bold")

                        ui.html('<svg id="barcode_register"></svg>')

                        #  Cancel button
                        ui.button(
                            icon="close", on_click=self.register_dialog.close
                        ).props("flat round").classes("!text-gray-700")

                    # input fields
                    with ui.column().classes("w-full h-full gap-8") as input_fields:

                        # Row 1
                        with ui.row().classes("w-full justify-between"):
                            # asset id input (disabled)
                            self.fields_add["asset_id"] = (
                                ui.input("Asset ID")
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            #  asset tag input (disabled)
                            self.fields_add["asset_tag"] = (
                                ui.input("Asset Tag")
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )
                            self.fields_add["asset_id"].disable()
                            self.fields_add["asset_tag"].disable()

                        # Row 2
                        with ui.row().classes("w-full justify-between"):
                            # serial number input
                            self.fields_add["serial_number"] = (
                                ui.input(
                                    "Serial Number",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            #  status drop down select
                            self.fields_add["status"] = (
                                ui.select(
                                    options=[
                                        "Active",
                                        "Repair",
                                        "In Inventory",
                                        "Decommissioned",
                                        "Missing",
                                        "Scrapped",
                                    ],
                                    label="Status",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                        # Row 3
                        with ui.row().classes("w-full justify-between"):
                            # brand input
                            self.fields_add["brand"] = (
                                ui.input(
                                    "Brand Name",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            # model input
                            self.fields_add["model"] = (
                                ui.input(
                                    "Model",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                        # Row 4
                        with ui.row().classes("w-full justify-between"):

                            # employee ID input
                            self.fields_add["employee_id"] = (
                                ui.input(
                                    "Employee ID",
                                    validation={
                                        "Required field": lambda value: (
                                            True
                                            if (
                                                self.fields_add["status"].value
                                                in ["Active", "Repair"]
                                                and value
                                            )
                                            or (
                                                self.fields_add["status"].value
                                                not in ["Active", "Repair"]
                                                and not value
                                            )
                                            else None
                                        ),
                                        "Employee does not exist": lambda value: (
                                            True
                                            if (
                                                (
                                                    self.fields_add["status"].value
                                                    in ["Active", "Repair"]
                                                    and self.load_employee(
                                                        self.fields_add, value
                                                    )
                                                )
                                                or (
                                                    self.fields_add["status"].value
                                                    not in ["Active", "Repair"]
                                                    and (not value)
                                                )
                                            )
                                            else None
                                        ),
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            # department input (disabled - auto generated when employee is keyed in)
                            self.fields_add["employee_name"] = (
                                ui.input(
                                    "Employee Name",
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_add["employee_name"].disable()

                        # row 5
                        with ui.row().classes("w-full justify-between"):
                            # department input (disabled - auto generated when employee is keyed in)
                            self.fields_add["department"] = (
                                ui.input(
                                    "Department",
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_add["date"] = (
                                ui.date_input("Purchase Date")
                                .classes("w-9/20")
                                .props("outlined hide-bottom-space clearable")
                            )

                            self.fields_add["department"].disable()

                with ui.tab_panel(tab_note):

                    self.fields_add["note"] = (
                        ui.textarea("Note")
                        .classes("w-full h-3/5")
                        .props(
                            'outlined hide-bottom-space clearable maxlength=200 autocomplete="off"'
                        )
                    )

            # cancel / save
            with ui.row().classes("w-full justify-end gap-2 mt-4"):

                # cancel button
                ui.button("Cancel", on_click=lambda: self.reset_add()).props(
                    "flat"
                ).classes("!text-gray-700")
                # save button
                save_button = ui.button(
                    "Save", on_click=lambda: self.save_asset()
                ).classes("!bg-blue-600 text-white hover:!bg-blue-700")

            # validate on popup load
            asyncio.create_task(self.validate(self.fields_add, save_button))
            # validate all the input fields
            self.fields_add["serial_number"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_add, save_button)
                ),
            )
            self.fields_add["status"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.disable_fields(self.fields_add, save_button)
                ),
            )
            self.fields_add["brand"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_add, save_button)
                ),
            )
            self.fields_add["model"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_add, save_button)
                ),
            )
            self.fields_add["employee_id"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_add, save_button)
                ),
            )

    def build_query(self):

        # |-----------query a record-----------|
        # query / edit an assets popup form
        with self.query_dialog, ui.card().classes(
            "w-3/5 h-full p-6 overflow-auto"
        ).style("max-width: none; max-height:none"):

            with ui.tabs().classes("w-full") as tabs:
                tab_general = ui.tab("General")
                tab_note = ui.tab("Note")

            with ui.tab_panels(tabs, value=tab_general).classes("w-full h-full"):

                with ui.tab_panel(tab_general):

                    # header row
                    with ui.row().classes("w-full justify-between items-center"):
                        # Title
                        ui.label("Edit Employee").classes("text-2xl font-bold")

                        ui.html('<svg id="barcode_query"></svg>')
                        #  Cancel button
                        ui.button(icon="close", on_click=self.query_dialog.close).props(
                            "flat round"
                        ).classes("!text-gray-700")

                    # input fields
                    with ui.column().classes("w-full gap-8") as input_fields:

                        # Row 1
                        with ui.row().classes("w-full justify-between"):
                            # asset id input (disabled)
                            self.fields_edit["asset_id"] = (
                                ui.input("Asset ID")
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            #  asset tag input (disabled)
                            self.fields_edit["asset_tag"] = (
                                ui.input(
                                    "Asset Tag",
                                    validation={
                                        "Asset tag does not exist": lambda value: (
                                            True
                                            if (self.asset_tag_exists(value) and value)
                                            else None
                                        ),
                                        "Required field": lambda value: (
                                            True if value else None
                                        ),
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_edit["asset_id"].disable()

                        # Row 2
                        with ui.row().classes("w-full justify-between"):
                            # serial number input
                            self.fields_edit["serial_number"] = (
                                ui.input(
                                    "Serial Number",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            #  status drop down select
                            self.fields_edit["status"] = (
                                ui.select(
                                    options=[
                                        "Active",
                                        "Repair",
                                        "In Inventory",
                                        "Decommissioned",
                                        "Missing",
                                        "Scrapped",
                                    ],
                                    label="Status",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )
                            self.fields_edit["serial_number"].disable()
                            self.fields_edit["status"].disable()

                        # Row 3
                        with ui.row().classes("w-full justify-between"):
                            # brand input
                            self.fields_edit["brand"] = (
                                ui.input(
                                    "Brand Name",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            # model input
                            self.fields_edit["model"] = (
                                ui.input(
                                    "Model",
                                    validation={
                                        "Required field": lambda value: (
                                            True if value else None
                                        )
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )
                            self.fields_edit["brand"].disable()
                            self.fields_edit["model"].disable()

                        # Row 4
                        with ui.row().classes("w-full justify-between"):

                            # employee ID input
                            self.fields_edit["employee_id"] = (
                                ui.input(
                                    "Employee ID",
                                    validation={
                                        "Required field": lambda value: (
                                            True
                                            if (
                                                self.fields_edit["status"].value
                                                in ["Active", "Repair"]
                                                and value
                                            )
                                            or (
                                                self.fields_edit["status"].value
                                                not in ["Active", "Repair"]
                                                and not value
                                            )
                                            else None
                                        ),
                                        "Employee does not exist": lambda value: (
                                            True
                                            if (
                                                (
                                                    self.fields_edit["status"].value
                                                    in ["Active", "Repair"]
                                                    and self.load_employee(
                                                        self.fields_edit, value
                                                    )
                                                )
                                                or (
                                                    self.fields_edit["status"].value
                                                    not in ["Active", "Repair"]
                                                    and (not value)
                                                )
                                            )
                                            else None
                                        ),
                                    },
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            # department input (disabled - auto generated when employee is keyed in)
                            self.fields_edit["employee_name"] = (
                                ui.input(
                                    "Employee Name",
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_edit["employee_name"].disable()
                            self.fields_edit["employee_id"].disable()

                        # row 5
                        with ui.row().classes("w-full justify-between"):
                            # department input (disabled - auto generated when employee is keyed in)
                            self.fields_edit["department"] = (
                                ui.input(
                                    "Department",
                                )
                                .classes("w-9/20")
                                .props(
                                    'outlined hide-bottom-space clearable autocomplete="off"'
                                )
                            )

                            self.fields_edit["date"] = (
                                ui.date_input("Issued Date")
                                .classes("w-9/20")
                                .props(
                                    "outlined hide-bottom-space clearable autocomplete='off'"
                                )
                            )

                            self.fields_edit["department"].disable()
                            self.fields_edit["date"].disable()

                with ui.tab_panel(tab_note):

                    self.fields_edit["note"] = (
                        ui.textarea("Note")
                        .classes("w-full h-3/5")
                        .props(
                            'outlined hide-bottom-space clearable maxlength=200 autocomplete="off"'
                        )
                    )

            # separator
            ui.separator().style("background-color: gray; height: 1px;")

            # timestamp
            with ui.column().classes("w-full gap-8 p-4"):

                with ui.row():
                    ui.label("Last Updated :").classes(
                        "text-base font-semibold text-gray-800"
                    )

                    self.fields_edit["timestamp"] = ui.label("").classes(
                        "text-base text-gray-400 font-medium"
                    )

            # cancel / save
            with ui.row().classes("w-full justify-end gap-2 mt-4"):

                # cancel button
                ui.button("Cancel", on_click=self.query_dialog.close).props(
                    "flat"
                ).classes("!text-gray-700")
                # save button
                save_button = ui.button(
                    "Save", on_click=lambda: self.save_edit()
                ).classes("!bg-blue-600 text-white hover:!bg-blue-700")

            # validate on popup load
            asyncio.create_task(self.validate(self.fields_edit, save_button))

            # validate name and employee id
            # validate all the input fields
            self.fields_edit["serial_number"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )
            self.fields_edit["status"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.disable_fields(self.fields_edit, save_button)
                ),
            )

            self.fields_edit["brand"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )
            self.fields_edit["model"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )
            self.fields_edit["employee_id"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )
            self.fields_edit["date"].on(
                "update:model-value",
                lambda e: asyncio.create_task(
                    self.validate(self.fields_edit, save_button)
                ),
            )

    def build_main_page(self):

        # build side bar
        UIHelpers.build_side_bar()

        with ui.column().classes("w-full h-screen items-center justify-center gap-8"):

            ui.label("Asset Management System").classes(
                "text-4xl font-bold text-gray-800"
            )

            ui.label("Select an option").classes("text-lg text-gray-500")

            # --- Grid ---
            with ui.grid(columns=3).classes("gap-8 w-2/3"):

                def nav_card(icon, label, action):
                    with ui.button(on_click=action).props("flat").classes(
                        "h-48 w-full border-2 border-black rounded-xl "
                        "hover:shadow-lg hover:bg-gray-50 transition"
                    ):
                        with ui.column().classes("items-center justify-center gap-2"):

                            ui.icon(icon).classes("text-4xl text-gray-700 p-3")
                            ui.label(label).classes(
                                "text-gray-700 mt-2 text-lg text-center"
                            )

                nav_card("add_box", "Register Asset", self.open_register_dialog)
                nav_card("search", "Query Asset", self.open_query_dialog)
                nav_card(
                    "group", "Employee Listing", lambda: ui.navigate.to("/employees")
                )
                nav_card(
                    "apartment",
                    "Department Listing",
                    lambda: ui.navigate.to("/departments"),
                )
                nav_card(
                    "inventory_2", "Asset Listing", lambda: ui.navigate.to("/assets")
                )

    def build(self):

        ui.add_head_html(
            """
        <script src="https://cdn.jsdelivr.net/npm/jsbarcode@3.11.6/dist/JsBarcode.all.min.js"></script>
        """
        )

        # initialise UIhelpers
        UIHelpers().init()

        self.build_register()

        self.build_query()

        self.build_main_page()


ui.run()
